#![allow(unused)]

mod tables;

/// Building a qr code generator:
/// Steps:
/// 1. Analyze data to determine the optimal data encoding mode
/// 2. Encode data based on the encoding mode
/// 3. Turn encoded data and other segments into codewords
use bitstream_io::{BigEndian, BitWrite, BitWriter, LittleEndian}; // TODO: build our own

enum ErrorCorrection {
    Low,      // Recovers 7% of data
    Medium,   // Recovers 15% of data
    Quantile, // Recovers 25% of data
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

fn determine_encoding_mode(input: &str) -> Encoding {
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
    writer.into_writer()
}

struct Bitstream {
    pub data: Vec<u8>,
}

impl Bitstream {
    pub fn new() -> Self {
        Self {
            data: Vec::new(),
        }
    }

    pub fn write<T: num::Integer + Into<u128>>(&mut self, bit_length: u32, data: T) {
        // data is x bits long
        // we want to write num_bits to our stream
        // making sure that leading bytes are at the front
        let mut copy: u128 = data.into();
        let mut required_bits = 0;
        while copy > 0 {
            copy = copy >> 1;
            required_bits += 1;
        }
    }
}

fn numeric_encode(input: &str) -> (Vec<u8>, u32) {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);
    let mut total_bits = 0;
    let mut i = 0;
    let mut ex = Bitstream::new();
    while i < input.len() {
        let end = std::cmp::min(i + 3, input.len());
        let group = &input[i..end];
        let value = group.parse::<u16>().unwrap_or_default();
        let num_bits = match group.len() {
            3 => 10,
            2 => 7,
            _ => 4,
        };
        println!("{value}");
        // bitstream_io doesn't seem to write leading 0s, but we really need that
        bitstream.write(num_bits, value).unwrap();
        ex.write(num_bits, value);
        total_bits += num_bits;
        i += 3;
    }

    bitstream.byte_align().unwrap();
    (bitstream.into_writer(), total_bits)
}

fn alphanumeric_encode(input: &str) -> Vec<u8> {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);

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
        i += 2;
    }

    bitstream.into_writer()
}

fn byte_encode(input: &str) -> Vec<u8> {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);

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

        bitstream.write(num_bytes * 8, value).unwrap();
    }

    bitstream.into_writer()
}

// TODO: cleanup please
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
        ErrorCorrection::Quantile => 2,
        ErrorCorrection::High => 3,
    }
}

fn get_character_capacity(level: &ErrorCorrection, mode: &Encoding, version: usize) -> u16 {
    tables::CHARACTER_CAPACITIES[version - 1][correction_index(level)][mode_index(mode)]
}

// Minimum qr version that can hold the data
fn determine_version(level: &ErrorCorrection, mode: &Encoding, num_chars: usize) -> usize {
    let mut version = 1;
    loop {
        let capacity = get_character_capacity(&level, &mode, version);
        if num_chars <= capacity as usize {
            break;
        }
        version += 1;
    }
    version
}

fn determine_count_bits_size(version: usize, mode: &Encoding) -> u32 {
    let index = mode_index(mode);
    if version >= 1 && version <= 9 {
        return tables::COUNT_SIZES[0][index];
    } else if version >= 10 && version <= 26 {
        return tables::COUNT_SIZES[1][index];
    }
    tables::COUNT_SIZES[2][index]
}

fn determine_required_bit_length(version: usize, level: &ErrorCorrection) -> u32 {
    let i = correction_index(&level);
    tables::ECC_DATA[version - 1][i][0] * 8
}

fn encode_data(input: &str, level: ErrorCorrection) -> Vec<u8> {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);

    let mode = determine_encoding_mode(input);
    let mode_indicator = match mode {
        Encoding::Numeric => 1,
        Encoding::Alphanumeric => 2,
        Encoding::Byte => 4,
    };

    let version = determine_version(&level, &mode, input.len());
    let count_bit_size = determine_count_bits_size(version, &mode);
    let (data, data_size) = match mode {
        Encoding::Numeric => numeric_encode(input),
        _ => (Vec::new(), 0),
        //Encoding::Alphanumeric => alphanumeric_encode(input),
        //Encoding::Byte => byte_encode(input),
    };
    let mut x = data_size;

    let required = determine_required_bit_length(version, &level);
    let terminator_length = std::cmp::min(required - data_size, 4);

    bitstream.write(4, mode_indicator);
    bitstream.write(count_bit_size, input.len() as u32);
    println!("{:?}", data);
    for byte in data {
        let bits = std::cmp::min(8, x);
        bitstream.write(bits, byte);
        if x > 8 { x -= 8; };
    }
    bitstream.write(terminator_length, 0);

    // Pad with zeroes to make it a multiple of 8
    let mut length_so_far = data_size + terminator_length + count_bit_size + 4;
    if length_so_far % 8 != 0 {
        let next_mutliple = length_so_far / 8 * 8 + 8;
        let remaining = next_mutliple - length_so_far;
        for _ in 0..remaining {
            bitstream.write_bit(false);
            length_so_far += 1;
        }
    }

    // Add pad bytes if the bitstream is still up to the required length
    let remaining_bytes = (required - length_so_far) / 8;
    for i in 0..remaining_bytes {
        if i % 2 == 0 {
            bitstream.write(8, 236);
        } else {
            bitstream.write(8, 17);
        }
    }

    bitstream.into_writer()
}

#[cfg(test)]
mod test {
    use bitstream_io::BigEndian;

    use crate::*;

    #[test]
    fn test_data_analyzing() {
        assert_eq!(determine_encoding_mode("Hello world!"), Encoding::Byte);
        assert_eq!(
            determine_encoding_mode("HELLO WORLD 123 :/"),
            Encoding::Alphanumeric
        );
        assert_eq!(determine_encoding_mode("09865456789"), Encoding::Numeric);
        assert_eq!(determine_encoding_mode("aЉ윇😱"), Encoding::Byte);
        assert_eq!(determine_encoding_mode(""), Encoding::Byte);
    }

    /*
    fn test_encoding<T: bitstream_io::Numeric>(
        mode: Encoding,
        values: &[&str],
        expecteds: &[T],
        expected_lengths: &[u32],
    ) {
        for i in 0..values.len() {
            let stream = match mode {
                Encoding::Alphanumeric => alphanumeric_encode(values[i]),
                Encoding::Numeric => numeric_encode(values[i]),
                Encoding::Byte => byte_encode(values[i]),
            };
            let expected = create_bitstream(expected_lengths[i], expecteds[i]);
            assert_eq!(stream, expected);
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
    */

    // TODO: add more tests!!!!!
    #[test]
    fn test_data_encoding() {
        /*
        let bytes = encode_data("hello!", ErrorCorrection::Low);
        let expected = [
            0x40, 0x66, 0x86, 0x56, 0xC6, 0xC6, 0xF2, 0x10, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11,
            0xEC, 0x11, 0xEC, 0x11, 0xEC,
        ];
        assert_eq!(bytes, expected);
        */

        let bytes = encode_data("123", ErrorCorrection::Low);
        let expected = [
            0x10, 0x0C, 0x7B, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC
        ];
        assert_eq!(bytes, expected);
    }
}

fn main() {}
