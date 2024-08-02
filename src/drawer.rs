use crate::encoder;
use crate::tables;

pub struct QR {
    pub size: usize,
    pub matrix: Vec<u8>,

    matrix_copy: Vec<u8>,
    version: usize,
    level: encoder::ErrorCorrection,
    data: Vec<u8>,
    byte_index: usize,
    bit_index: usize,
}

impl QR {
    pub fn create(input: &str, level: encoder::ErrorCorrection) -> Self {
        let mode = encoder::get_encoding_mode(input);
        let version = encoder::get_version(level, mode, input.len());

        let size = 21 + (version - 1) * 4;
        let mut qr = Self {
            byte_index: 0,
            bit_index: 0,
            data: encoder::assemble_qr_data(input, level),
            matrix: vec![128; size * size],
            matrix_copy: vec![128; size * size],
            version,
            size,
            level,
        };

        qr.draw_initial_patterns();
        qr.draw_dummy_format_areas();
        qr.draw_version_info();
        qr.draw_data();
        qr
    }

    fn draw_module(&mut self, x: usize, y: usize, color: u8) {
        self.matrix[y * self.size + x] = color;
    }

    fn get_module(&self, x: usize, y: usize) -> u8 {
        self.matrix[y * self.size + x]
    }

    fn draw_finder_pattern(&mut self, startx: usize, starty: usize) {
        // Draw separators keeping in mind the finder pattern pattern positions in the matrix
        let is_right = startx > self.size / 2;
        let is_bottom = starty > self.size / 2;
        let separator_x = if is_right { 0 } else { 7 };
        let separator_y = if is_bottom { 0 } else { 7 };
        for i in 0..8 {
            self.draw_module(startx + i, starty + separator_y, 255);
            self.draw_module(startx + separator_x, starty + i, 255);
        }

        // Draw finder pattern
        for y in 0..7 {
            for x in 0..7 {
                let is_border = x == 0 || y == 0 || x == 6 || y == 6;
                let is_inner = x >= 2 && x <= 4 && y >= 2 && y <= 4;
                let color = if is_border || is_inner { 0 } else { 255 };
                let real_x = startx + x + usize::from(is_right);
                let real_y = starty + y + usize::from(is_bottom);
                self.draw_module(real_x, real_y, color);
            }
        }
    }

    fn draw_alignment_patterns(&mut self) {
        // Center x and y positions of our alignment patterns
        let alignment_positions = tables::get_alignment_pattern_locations(self.version);

        for a in 0..alignment_positions.len() {
            for b in 0..alignment_positions.len() {
                let cx = alignment_positions[b];
                let cy = alignment_positions[a];

                // Can't draw alignment pattern over top the finder patterns
                let in_top_left = cx < 8 && cy < 8;
                let in_top_right = cx >= self.size - 8 && cy < 8;
                let in_bottom_left = cx < 8 && cy >= self.size - 8;
                if in_top_left || in_top_right || in_bottom_left {
                    continue;
                }

                // Draw a mini finder pattern
                for i in 0..5 {
                    for j in 0..5 {
                        let x = (cx - 2) + j;
                        let y = (cy - 2) + i;
                        let is_center = x == cx && y == cy;
                        let is_border = i == 0 || i == 4 || j == 0 || j == 4;
                        self.draw_module(x, y, if is_border || is_center { 0 } else { 255 });
                    }
                }
            }
        }
    }

    fn draw_initial_patterns(&mut self) {
        // Draw horizantal and vertical timing patterns
        for i in 0..self.size {
            let color = if i % 2 == 0 { 0 } else { 255 };
            self.draw_module(i, 6, color);
            self.draw_module(6, i, color);
        }

        self.draw_alignment_patterns();

        // Draw finder pattern at top left, top right and bottom left corners
        self.draw_finder_pattern(0, 0);
        self.draw_finder_pattern(0, self.size - 8);
        self.draw_finder_pattern(self.size - 8, 0);
    }

    fn draw_dummy_format_areas(&mut self) {
        // Draw the dark module
        self.draw_module(8, 4 * self.version + 9, 0);

        // Draw reserved areas adjacent to the finder patterns
        self.draw_module(8, 8, 255);
        for i in 0..8 {
            let pos = self.size - i - 1;
            let x_positions = [8, self.size - i - 1, i, 8];
            let y_positions = [pos, 8, 8, i];

            for j in 0..4 {
                if self.get_module(x_positions[j], y_positions[j]) == 128 {
                    self.draw_module(x_positions[j], y_positions[j], 255);
                }
            }
        }
    }

    fn draw_version_info(&mut self) {
        if self.version < 7 {
            return;
        }

        let bitstring = tables::get_version_bitstring(self.version);
        let mut index = 0;
        for x in 0..6 {
            for y in 0..3 {
                let color = if bitstring[index] == 0 { 255 } else { 0 };
                self.draw_module(5 - x, self.size - 9 - y, color);
                self.draw_module(self.size - 9 - y, 5 - x, color);
                index += 1;
            }
        }
    }

    fn get_color(&self, bit: u8) -> u8 {
        if bit == 0 {
            return 255;
        }
        0
    }

    fn draw_format_info(&mut self, mask_index: usize) {
        let mut x = 0;
        let mut y = (self.size - 1) as i32;
        let bits = tables::FORMAT_INFO_BITS[self.level as usize][mask_index];

        self.draw_module(self.size - 8, 8, self.get_color(bits[7]));
        for i in 0..15 {
            // Draw vertically
            self.draw_module(8, y as usize, self.get_color(bits[i]));
            y = if i == 6 { 8 } else { y - 1 }; // Skip middle vertical gap
            y = if y == 6 { 5 } else { y }; // Skip timing pattern

            // Draw horizantally
            self.draw_module(x, 8, self.get_color(bits[i]));
            x = if i == 7 { self.size - 7 } else { x + 1 }; // Skip middle horizantal gap
            x = if x == 6 { 7 } else { x }; // Skip timing pattern
        }
    }

    fn get_next_bit(&mut self) -> u8 {
        if self.byte_index >= self.data.len() {
            return 0;
        }

        let shifted = self.data[self.byte_index] << self.bit_index & 0x80;
        let bit = if shifted == 128 { 1 } else { 0 };

        self.bit_index += 1;
        if self.bit_index == 8 {
            self.byte_index += 1;
            self.bit_index = 0;
        }

        bit
    }

    // Return 1 if the mask applies else 0
    fn get_mask_rule(&self, x: usize, y: usize, mask_index: usize) -> u8 {
        u8::from(match mask_index {
            0 => (x + y) % 2 == 0,
            1 => y % 2 == 0,
            2 => x % 3 == 0,
            3 => (x + y) % 3 == 0,
            4 => (((y as f64) / 2.0).floor() + ((x as f64) / 3.0).floor()) % 2.0 == 0.0,
            5 => (((x * y) % 2) + ((x * y) % 3)) == 0,
            6 => (((x * y) % 2) + ((x * y) % 3)) % 2 == 0,
            7 => (((x + y) % 2) + ((x * y) % 3)) % 2 == 0,
            _ => false,
        })
    }

    fn draw_and_mask_data_bits(&mut self, mask_index: usize) {
        let size = self.size as i32;
        let mut x = size - 1;
        let mut y = size - 1;
        let mut going_up = true;
        let num_bits = self.data.len() * 8 + tables::get_remainder_bit_count(self.version);

        while x > 0 {
            // Skip the top timing pattern
            if y == 6 && x >= 9 && x <= size - 8 {
                y = if going_up { y - 1 } else { y + 1 };
            }

            // Skip the side timing pattern
            if x == 6 && y >= 9 && y <= size - 8 {
                x -= 1;
            }

            // Draw right to left
            for i in 0..2 {
                let p = x - i;
                if p >= 0 && self.get_module(p as usize, y as usize) == 128 {
                    let mask = self.get_mask_rule(p as usize, y as usize, mask_index);
                    let bit = self.get_next_bit() ^ mask;
                    self.draw_module(p as usize, y as usize, self.get_color(bit));
                }
            }

            // Go up and down
            y = if going_up { y - 1 } else { y + 1 };
            if y == -1 || y == size {
                y = if y == -1 { 0 } else { size - 1 };
                going_up = !going_up;
                x -= 2;
            }
        }
    }

    fn compute_penalty_score(&mut self) {
        todo!("implement penalty rules!");
    }

    fn draw_data(&mut self) {
        self.matrix_copy = self.matrix.clone();
        for mask in 0..8 {
            self.matrix = self.matrix_copy.clone();
            self.draw_and_mask_data_bits(mask);
            self.draw_format_info(mask);
            self.compute_penalty_score();
            break;
        }
    }
}
