#![allow(unused)]

mod tables;
use std::collections::VecDeque;
use bitstream_io::{BigEndian, BitWrite, BitWriter};

enum ErrorCorrection {
    Low,      // Recovers 7% of data
    Medium,   // Recovers 15% of data
    Quartile, // Recovers 25% of data
    High,     // Recovers 30% of data
}

#[derive(Debug, PartialEq)]
enum Encoding {
    Numeric,
    Alphanumeric,
    Byte,
}

fn is_alphanumeric(codepoint: char) -> bool {
    match codepoint {
        '0'..='9' => true,
        'A'..='Z' => true,
        ' ' => true,
        '$' => true,
        '%' => true,
        '*' => true,
        '+' => true,
        '-' => true,
        '.' => true,
        '/' => true,
        ':' => true,
        _ => false,
    }
}

fn alphanumeric_value(codepoint: char) -> u16 {
    match codepoint {
        'A'..='Z' => (codepoint as u16 - 65) + 10,
        '0'..='9' => codepoint as u16 - 48,
        ' ' => 36,
        '$' => 37,
        '%' => 38,
        '*' => 39,
        '+' => 40,
        '-' => 41,
        '.' => 42,
        '/' => 43,
        ':' => 44,
        _ => 0,
    }
}

fn get_encoding_mode(input: &str) -> Encoding {
    let mut numeric = false;
    let mut alphanumeric = false;

    for codepoint in input.chars() {
        numeric = codepoint.is_ascii_digit();
        alphanumeric = is_alphanumeric(codepoint);
    }

    if numeric {
        return Encoding::Numeric;
    } else if alphanumeric {
        return Encoding::Alphanumeric;
    }

    Encoding::Byte
}

fn create_bitstream<T: bitstream_io::Numeric>(bits: u32, value: T) -> Vec<u8> {
    let mut writer = BitWriter::endian(Vec::new(), BigEndian);
    writer.write(bits, value).unwrap();
    writer.byte_align().unwrap();
    writer.into_writer()
}

struct EncodedData {
    data: Vec<u8>,
    size_in_bits: u32,
}

impl EncodedData {
    fn new() -> Self {
        Self {
            data: Vec::new(),
            size_in_bits: 0,
        }
    }
}

fn numeric_encode(input: &str) -> EncodedData {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);
    let mut encoded = EncodedData::new();
    let mut i = 0;
    while i < input.len() {
        let end = std::cmp::min(i + 3, input.len());
        let group = &input[i..end];
        let value = group.parse::<u16>().unwrap_or_default();
        let num_bits = match group.len() {
            3 => 10,
            2 => 7,
            _ => 4,
        };
        bitstream.write(num_bits, value).unwrap();
        encoded.size_in_bits += num_bits;
        i += 3;
    }

    bitstream.byte_align().unwrap();
    encoded.data = bitstream.into_writer();
    encoded
}

fn alphanumeric_encode(input: &str) -> EncodedData {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);
    let mut encoded = EncodedData::new();
    let mut i = 0;
    while i < input.len() {
        let end = std::cmp::min(i + 2, input.len());
        let pair = &input[i..end];

        let mut num_bits = 6;
        let mut value = alphanumeric_value(pair.chars().nth(0).unwrap());

        if pair.len() == 2 {
            let last = alphanumeric_value(pair.chars().nth(1).unwrap());
            value = value * 45 + last;
            num_bits = 11;
        }

        bitstream.write(num_bits, value).unwrap();
        encoded.size_in_bits += num_bits;
        i += 2;
    }

    bitstream.byte_align().unwrap();
    encoded.data = bitstream.into_writer();
    encoded
}

fn byte_encode(input: &str) -> EncodedData {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);
    let mut encoded = EncodedData::new();
    for codepoint in input.chars() {
        let num_bytes = codepoint.len_utf8() as u32;
        let mut bytes = [0u8; 4];
        codepoint.encode_utf8(&mut bytes);

        // Big endian encode
        let mut value = 0;
        for i in 0..num_bytes as usize {
            let index = num_bytes as usize - i - 1;
            value |= (bytes[i] as u32) << index * 8;
        }

        let num_bits = num_bytes * 8;
        bitstream.write(num_bits, value).unwrap();
        encoded.size_in_bits += num_bits;
    }

    bitstream.byte_align().unwrap();
    encoded.data = bitstream.into_writer();
    encoded
}

fn mode_index(mode: &Encoding) -> usize {
    match mode {
        Encoding::Numeric => 0,
        Encoding::Alphanumeric => 1,
        Encoding::Byte => 2,
    }
}

fn correction_index(level: &ErrorCorrection) -> usize {
    match level {
        ErrorCorrection::Low => 0,
        ErrorCorrection::Medium => 1,
        ErrorCorrection::Quartile => 2,
        ErrorCorrection::High => 3,
    }
}

// Get the minimum qr version that can hold the data
fn get_version(level: &ErrorCorrection, mode: &Encoding, num_chars: usize) -> usize {
    let mut version = 1;
    loop {
        let capacity = tables::get_character_capacity(correction_index(level), mode_index(mode), version);
        if num_chars <= capacity as usize {
            break;
        }
        version += 1;
    }
    version
}

fn encode_data(input: &str, level: &ErrorCorrection) -> Vec<u8> {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);

    let mode = get_encoding_mode(input);
    let mode_indicator = match mode {
        Encoding::Numeric => 1,
        Encoding::Alphanumeric => 2,
        Encoding::Byte => 4,
    };

    let version = get_version(&level, &mode, input.len());
    let count_bit_size = tables::get_count_bits_size(version, mode_index(&mode));
    let encoded_data = match mode {
        Encoding::Numeric => numeric_encode(input),
        Encoding::Alphanumeric => alphanumeric_encode(input),
        Encoding::Byte => byte_encode(input),
    };

    let mode_size = 4;
    let required_size = tables::get_required_bit_length(version, correction_index(&level));
    let terminator_size = std::cmp::min(required_size - encoded_data.size_in_bits, 4);

    // Write mode and count bits
    bitstream.write(mode_size, mode_indicator);
    bitstream.write(count_bit_size, input.len() as u32);

    // Write the correct amount of data in bits to the bitstream
    let mut size = encoded_data.size_in_bits;
    for mut byte in encoded_data.data {
        let bits = std::cmp::min(8, size);
        if bits < 8 { byte >>= 8 - bits; }
        bitstream.write(bits, byte);
        if size > 8 { size -= 8; };
    }

    bitstream.write(terminator_size, 0);

    // Pad with zeroes to make the bitstream's size a multiple of 8
    let mut length_so_far = encoded_data.size_in_bits + terminator_size + count_bit_size + mode_size;
    if length_so_far % 8 != 0 {
        let next_mutliple = length_so_far / 8 * 8 + 8;
        let remaining = next_mutliple - length_so_far;
        for _ in 0..remaining {
            bitstream.write_bit(false);
            length_so_far += 1;
        }
    }

    // Add pad bytes if the bitstream is still smaller than the required length
    let remaining_bytes = (required_size - length_so_far) / 8;
    for i in 0..remaining_bytes {
        if i % 2 == 0 {
            bitstream.write(8, 236);
        } else {
            bitstream.write(8, 17);
        }
    }

    bitstream.into_writer()
}

// Use the Reed-Solomon algorithm to generate error correction codes
// for our encoded data
fn generate_error_correction_codes(data: &Vec<u8>, level: &ErrorCorrection, version: usize) -> Vec<u8> {
    let data_codeword_count = data.len();
    let ecc_count = tables::get_error_codeword_count(version, correction_index(&level));
    let length: usize = data_codeword_count + ecc_count;
    let generator_polynomial = tables::get_generator_polynomial(ecc_count);

    // Create a array with the data and `ecc_count` zeroes at the end
    let mut buffer = VecDeque::with_capacity(length);
    for codeword in data.clone() {
        buffer.push_back(codeword);
    }
    for _ in 0..ecc_count {
        buffer.push_back(0);
    }

    for i in 0..data_codeword_count {
        let mut generator = generator_polynomial.clone();
        let number = buffer.pop_front().unwrap(); // Get the first value and shift the buffer to the left
        if number == 0 {
            continue;
        }
        let exponent = tables::get_galois_antilog(number as usize);

        for j in 0..generator.len() {
            let real_value: u32 = generator_polynomial[j] as u32 + exponent as u32;
            let galois_value = (real_value % 255) as u8;
            let log = tables::get_galois_log(galois_value as usize);
            generator[j] = log ^ buffer[j];
        }

        // The generator is our new buffer
        buffer = generator.clone();
        if i == data_codeword_count - 1 {
            continue; // Don't pad the final buffer
        }
        // Pad with zeroes to fill the buffer back to its original length
        for _ in 0..length - buffer.len() {
            buffer.push_back(0);
        }
    }

    let mut error_correction_codewords = Vec::new();
    for byte in buffer {
        error_correction_codewords.push(byte);
    }
    error_correction_codewords
}

fn structure_qr_data(input: &str, level: &ErrorCorrection) -> Vec<u8> {
    let data = encode_data(input, level);
    let mode = get_encoding_mode(input);
    let version = get_version(level, &mode, input.len());

    let info = tables::ECC_DATA[version - 1][correction_index(level)];
    let group_block_counts = [info[1], info[3]];
    let codeword_counts = [info[2], info[4]];

    // Split the data into groups and blocks
    let mut data_groups: Vec<Vec<Vec<u8>>> = Vec::new();
    data_groups.push(Vec::new()); // First group
    data_groups.push(Vec::new()); // Second group
    let mut index = 0;
    for group in 0..2 {
        let num_blocks = group_block_counts[group];
        for _ in 0..num_blocks {
            let mut block = Vec::new();
            let codeword_count = codeword_counts[group];
            for _ in 0..codeword_count {
                block.push(data[index]);
                index += 1;
            }
            data_groups[group].push(block);
        }
    }

    // Generate error correction codes for each group
    let mut ecc_groups: Vec<Vec<Vec<u8>>> = Vec::new();
    ecc_groups.push(Vec::new());
    ecc_groups.push(Vec::new());
    for group in 0..2 {
        for block in data_groups[group].clone() {
            let codes = generate_error_correction_codes(&block, &level, version);
            ecc_groups[group].push(codes);
        }
    }

    let mut interleaved_data = Vec::new();

    // Interleave the data blocks
    let max_length = std::cmp::max(codeword_counts[0], codeword_counts[1]) as usize;
    for i in 0..max_length {
        for group in 0..2 {
            for block in data_groups[group].clone() {
                if i < block.len() {
                    interleaved_data.push(block[i]);
                }
            }
        }
    }

    // Interleave the error correction blocks
    for i in 0..info[0] as usize {
        for group in 0..2 {
            for block in ecc_groups[group].clone() {
                if i < block.len() {
                    interleaved_data.push(block[i]);
                }
            }
        }
    }

    interleaved_data
}

#[cfg(test)]
mod test {
    use bitstream_io::BigEndian;

    use crate::*;

    #[test]
    fn test_data_analyzing() {
        assert_eq!(get_encoding_mode("Hello world!"), Encoding::Byte);
        assert_eq!(
            get_encoding_mode("HELLO WORLD 123 :/"),
            Encoding::Alphanumeric
        );
        assert_eq!(get_encoding_mode("09865456789"), Encoding::Numeric);
        assert_eq!(get_encoding_mode("aЉ윇😱"), Encoding::Byte);
        assert_eq!(get_encoding_mode(""), Encoding::Byte);
    }

    fn test_encoding<T: bitstream_io::Numeric>(
        mode: Encoding,
        values: &[&str],
        expecteds: &[T],
        expected_lengths: &[u32],
    ) {
        for i in 0..values.len() {
            let encoded = match mode {
                Encoding::Alphanumeric => alphanumeric_encode(values[i]),
                Encoding::Numeric => numeric_encode(values[i]),
                Encoding::Byte => byte_encode(values[i]),
            };
            let expected = create_bitstream(expected_lengths[i], expecteds[i]);
            assert_eq!(encoded.size_in_bits, expected_lengths[i]);
            assert_eq!(encoded.data, expected);
        }
    }

    #[test]
    fn test_numeric_encoding() {
        let values = ["867", "1234567890", "00100308", "0"];
        let bits = [
            0b1101100011,
            0b0001111011011100100011000101010000u64,
            0b000000000100000000110001000u64,
            0b0000,
        ];
        let lengths = [10, 34, 27, 4];
        test_encoding(Encoding::Numeric, &values, &bits, &lengths)
    }

    #[test]
    fn test_alphanumeric_encoding() {
        let values = ["HELLO WORLD", " $%*+-./:1"];
        let bits = [
            0b0110000101101111000110100010111001011011100010011010100001101u64,
            0b1100111100111011010101111001100011111000110111110111101u64,
        ];
        let lengths = [61, 55];
        test_encoding(Encoding::Alphanumeric, &values, &bits, &lengths)
    }

    #[test]
    fn test_byte_encoding() {
        let values = ["hello", "aЉ윇😱", "qr! ;)"];
        let bits = [
            0b0110100001100101011011000110110001101111u128,
            0b01100001110100001000100111101100100111001000011111110000100111111001100010110001u128,
            0b011100010111001000100001001000000011101100101001u128,
        ];
        let lengths = [40, 80, 48];
        test_encoding(Encoding::Byte, &values, &bits, &lengths)
    }

    #[test]
    fn test_data_encoding() {
        let bytes = encode_data("hello!", &ErrorCorrection::Low);
        let expected = [
            0x40, 0x66, 0x86, 0x56, 0xC6, 0xC6, 0xF2, 0x10, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11,
            0xEC, 0x11, 0xEC, 0x11, 0xEC,
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("123", &ErrorCorrection::Low);
        let expected = [
            0x10, 0x0C, 0x7B, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("aЉ윇😱", &ErrorCorrection::Medium);
        let expected = [
            0x40, 0xA6, 0x1D, 0x08, 0x9E, 0xC9, 0xC8, 0x7F, 0x09, 0xF9, 0x8B, 0x10, 0xEC, 0x11, 0xEC, 0x11
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("", &ErrorCorrection::High);
        let expected = [
            0x40, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("LOREM IPSUM SIT DOLOR AMED", &ErrorCorrection::Quartile);
        let expected = [
            0x20, 0xD3, 0xC9, 0x99, 0xB0, 0x09, 0xA1, 0xD0, 0xA8, 0x05,
            0x3F, 0xA9, 0xEA, 0x61, 0x79, 0x33, 0x8C, 0xEC, 0x28, 0x30, 0xEC, 0x11
        ];
        assert_eq!(bytes, expected);
    }

    // TODO: the implementation is wrong
    #[test]
    fn test_error_correction_coding() {
        let level = ErrorCorrection::High;
        let version = 1;
        let bytes = vec![32, 65, 205, 69, 41, 220, 46, 128, 236];
        let correction_codes = generate_error_correction_codes(&bytes, &level, version);
        let expected = [42, 159, 74, 221, 244, 169, 239, 150, 138, 70, 237, 85, 224, 96, 74, 219, 61];
        assert_eq!(correction_codes, expected);

        let level = ErrorCorrection::Low;
        let version = 1;
        let bytes = vec![0x40, 0x56, 0x86, 0x56, 0xC6, 0xC6, 0xF0,
                                  0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11];
        let correction_codes = generate_error_correction_codes(&bytes, &level, version);
        let expected = [0x25, 0x19, 0xD0, 0xD2, 0x68, 0x59, 0x39];
        assert_eq!(correction_codes, expected);
    }

    #[test]
    fn structure_encoded_data() {
        let input = "hello";
        let level = &ErrorCorrection::Low;
        let data = structure_qr_data(input, level);
        let expected = [0x40, 0x56, 0x86, 0x56, 0xC6, 0xC6, 0xF0,
                                   0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC,
                                   0x11, 0xEC, 0x11, 0xEC, 0x11, 0x25, 0x19, 0xD0,
                                   0xD2, 0x68, 0x59, 0x39];
        assert_eq!(data, expected);
    }
}

fn main() {}
