#![allow(unused)]

mod tables;

/// Building a qr code generator:
/// Steps:
/// 1. Analyze data to get the optimal data encoding mode
/// 2. Encode data based on the encoding mode
/// 3. Turn encoded data and other segments into codewords
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

// Get the number of characters that can be held in a given qr code
fn get_character_capacity(level: &ErrorCorrection, mode: &Encoding, version: usize) -> u16 {
    tables::CHARACTER_CAPACITIES[version - 1][correction_index(level)][mode_index(mode)]
}

// Get the minimum qr version that can hold the data
fn get_version(level: &ErrorCorrection, mode: &Encoding, num_chars: usize) -> usize {
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

// Get the number of bits needed to encode the encoded data's size
fn get_count_bits_size(version: usize, mode: &Encoding) -> u32 {
    let index = mode_index(mode);
    if version >= 1 && version <= 9 {
        return tables::COUNT_SIZES[0][index];
    } else if version >= 10 && version <= 26 {
        return tables::COUNT_SIZES[1][index];
    }
    tables::COUNT_SIZES[2][index]
}

// Get the size that the encoded data is required to be
fn get_required_bit_length(version: usize, level: &ErrorCorrection) -> u32 {
    let i = correction_index(&level);
    tables::ECC_DATA[version - 1][i][0] * 8
}

fn encode_data(input: &str, level: ErrorCorrection) -> Vec<u8> {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);

    let mode = get_encoding_mode(input);
    let mode_indicator = match mode {
        Encoding::Numeric => 1,
        Encoding::Alphanumeric => 2,
        Encoding::Byte => 4,
    };

    let version = get_version(&level, &mode, input.len());
    let count_bit_size = get_count_bits_size(version, &mode);
    let encoded_data = match mode {
        Encoding::Numeric => numeric_encode(input),
        Encoding::Alphanumeric => alphanumeric_encode(input),
        Encoding::Byte => byte_encode(input),
    };

    let mode_size = 4;
    let required_size = get_required_bit_length(version, &level);
    println!("{required_size} {}", version);
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
        let bytes = encode_data("hello!", ErrorCorrection::Low);
        let expected = [
            0x40, 0x66, 0x86, 0x56, 0xC6, 0xC6, 0xF2, 0x10, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11,
            0xEC, 0x11, 0xEC, 0x11, 0xEC,
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("123", ErrorCorrection::Low);
        let expected = [
            0x10, 0x0C, 0x7B, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("aЉ윇😱", ErrorCorrection::Medium);
        let expected = [
            0x40, 0xA6, 0x1D, 0x08, 0x9E, 0xC9, 0xC8, 0x7F, 0x09, 0xF9, 0x8B, 0x10, 0xEC, 0x11, 0xEC, 0x11
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("", ErrorCorrection::High);
        let expected = [
            0x40, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("lorem ipsum sit dolor amed", ErrorCorrection::Quartile);
        let expected = [
            0x41, 0xA6, 0xC6, 0xF7, 0x26, 0x56, 0xD2, 0x06, 0x97, 0x07,
            0x37, 0x56, 0xD2, 0x07, 0x36, 0x97, 0x42, 0x06, 0x46, 0xF6,
            0xC6, 0xF7, 0x22, 0x06, 0x16, 0xD6, 0x56, 0x40, 0xEC, 0x11,
            0xEC, 0x11, 0xEC, 0x11
        ];
        assert_eq!(bytes, expected);
    }
}

fn main() {}
