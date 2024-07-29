use crate::tables;
use bitstream_io::{BigEndian, BitWrite, BitWriter};
use std::{collections::VecDeque, iter};

#[derive(Copy, Clone, PartialEq)]
pub enum ErrorCorrection {
    Low = 0,      // Recovers 7% of data
    Medium = 1,   // Recovers 15% of data
    Quartile = 2, // Recovers 25% of data
    High = 3,     // Recovers 30% of data
}

#[derive(Copy, Clone, PartialEq, Debug)]
enum EncodingMode {
    Numeric = 0,
    Alphanumeric = 1,
    Byte = 2,
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
        '0'..='9' => codepoint as u16 - 48,
        'A'..='Z' => (codepoint as u16 - 65) + 10,
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

fn get_encoding_mode(input: &str) -> EncodingMode {
    let mut numeric = false;
    let mut alphanumeric = false;

    for codepoint in input.chars() {
        numeric = codepoint.is_ascii_digit();
        alphanumeric = is_alphanumeric(codepoint);
    }

    if numeric {
        return EncodingMode::Numeric;
    } else if alphanumeric {
        return EncodingMode::Alphanumeric;
    }

    EncodingMode::Byte
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
        let num_bytes = codepoint.len_utf8();
        let mut bytes = [0u8; 4];
        codepoint.encode_utf8(&mut bytes);

        // Write the codepoint to a u32 with the most
        // significant byte coming first
        let mut value = 0;
        for i in 0..num_bytes {
            let index = num_bytes - i - 1;
            value |= (bytes[i] as u32) << index * 8;
        }

        let num_bits = num_bytes as u32 * 8;
        bitstream.write(num_bits, value).unwrap();
        encoded.size_in_bits += num_bits;
    }

    bitstream.byte_align().unwrap();
    encoded.data = bitstream.into_writer();
    encoded
}

// Get the minimum qr version that can hold the data
fn get_version(level: ErrorCorrection, mode: EncodingMode, num_chars: usize) -> usize {
    let mut version = 1;
    loop {
        let capacity = tables::CHARACTER_CAPACITIES[version - 1][level as usize][mode as usize];
        if num_chars <= capacity as usize {
            break;
        }
        version += 1;
    }
    version
}

// Get the number of bits needed to represent the encoded data's size
fn get_count_bits_size(version: usize, mode: EncodingMode) -> u32 {
    const COUNT_SIZES: [[u32; 3]; 3] = [[10, 9, 8], [12, 11, 16], [14, 13, 16]];
    let mode_index = mode as usize;

    if version >= 1 && version <= 9 {
        return COUNT_SIZES[0][mode_index];
    } else if version >= 10 && version <= 26 {
        return COUNT_SIZES[1][mode_index];
    }
    COUNT_SIZES[2][mode_index]
}

// Get the size in bits that the encoded data is required to be
fn get_required_bit_length(version: usize, level: ErrorCorrection) -> u32 {
    let values = tables::ECC_DATA[version - 1][level as usize];
    let block1_size = values[1] * values[2];
    let block2_size = values[3] * values[4];
    (block1_size + block2_size) * 8
}

// Return the number of error correction codewords in a block given the error
// correction level and version
fn get_error_codeword_count(version: usize, level: ErrorCorrection) -> usize {
    tables::ECC_DATA[version - 1][level as usize][0] as usize
}

fn encode_data(input: &str, level: ErrorCorrection) -> Vec<u8> {
    let mut bitstream = BitWriter::endian(Vec::new(), BigEndian);

    let mode = get_encoding_mode(input);
    let mode_indicator = match mode {
        EncodingMode::Numeric => 1,
        EncodingMode::Alphanumeric => 2,
        EncodingMode::Byte => 4,
    };

    let version = get_version(level, mode, input.len());
    let count_bit_size = get_count_bits_size(version, mode);
    let encoded_data = match mode {
        EncodingMode::Numeric => numeric_encode(input),
        EncodingMode::Alphanumeric => alphanumeric_encode(input),
        EncodingMode::Byte => byte_encode(input),
    };

    let mode_size = 4;
    let required_size = get_required_bit_length(version, level);
    let terminator_size = std::cmp::min(required_size - encoded_data.size_in_bits, 4);

    // Write mode and count bits
    bitstream.write(mode_size, mode_indicator);
    bitstream.write(count_bit_size, input.len() as u32);

    // Write the correct amount of data in bits to the bitstream
    let mut size = encoded_data.size_in_bits;
    for mut byte in encoded_data.data {
        let bits = std::cmp::min(8, size);
        if bits < 8 {
            // The byte is in big endian format, so we
            // need to shift right to align the signifcant
            // remaining bits to their least significant positions
            byte >>= 8 - bits;
        }
        bitstream.write(bits, byte);
        if size > 8 {
            size -= 8;
        }
    }

    bitstream.write(terminator_size, 0);

    // Pad with zeroes to make the bitstream's size in bits a multiple of 8
    let mut length_in_bits =
        encoded_data.size_in_bits + terminator_size + count_bit_size + mode_size;
    if length_in_bits % 8 != 0 {
        let next_mutliple = length_in_bits / 8 * 8 + 8;
        let remaining = next_mutliple - length_in_bits;
        for _ in 0..remaining {
            bitstream.write_bit(false);
            length_in_bits += 1;
        }
    }

    // Add pad bytes if the bitstream is still smaller than the required length
    let remaining_bytes = (required_size - length_in_bits) / 8;
    for i in 0..remaining_bytes {
        if i % 2 == 0 {
            bitstream.write(8, 236);
        } else {
            bitstream.write(8, 17);
        }
    }

    bitstream.byte_align().unwrap();
    bitstream.into_writer()
}

// Map the number of error correction keywords to their corresponding
// generator polynomials. The generator polynomials are represented as exponents
// of alpha. For example, the generator polynomial for 7 error correction codes
// would look like so: α0x7 + α87x6 + α229x5 + α146x4 + α149x3 + α238x2 + α102x + α21
fn get_generator_polynomial(count: usize) -> VecDeque<u8> {
    match count {
        7 => VecDeque::from([87, 229, 146, 149, 238, 102, 21]),
        10 => VecDeque::from([251, 67, 46, 61, 118, 70, 64, 94, 32, 45]),
        13 => VecDeque::from([74, 152, 176, 100, 86, 100, 106, 104, 130, 218, 206, 140, 78]),
        15 => VecDeque::from([
            8, 183, 61, 91, 202, 37, 51, 58, 58, 237, 140, 124, 5, 99, 105,
        ]),
        16 => VecDeque::from([
            120, 104, 107, 109, 102, 161, 76, 3, 91, 191, 147, 169, 182, 194, 225, 120,
        ]),
        17 => VecDeque::from([
            43, 139, 206, 78, 43, 239, 123, 206, 214, 147, 24, 99, 150, 39, 243, 163, 136,
        ]),
        18 => VecDeque::from([
            215, 234, 158, 94, 184, 97, 118, 170, 79, 187, 152, 148, 252, 179, 5, 98, 96, 153,
        ]),
        20 => VecDeque::from([
            17, 60, 79, 50, 61, 163, 26, 187, 202, 180, 221, 225, 83, 239, 156, 164, 212, 212, 188,
            190,
        ]),
        22 => VecDeque::from([
            210, 171, 247, 242, 93, 230, 14, 109, 221, 53, 200, 74, 8, 172, 98, 80, 219, 134, 160,
            105, 165, 231,
        ]),
        24 => VecDeque::from([
            229, 121, 135, 48, 211, 117, 251, 126, 159, 180, 169, 152, 192, 226, 228, 218, 111, 0,
            117, 232, 87, 96, 227, 21,
        ]),
        26 => VecDeque::from([
            173, 125, 158, 2, 103, 182, 118, 17, 145, 201, 111, 28, 165, 53, 161, 21, 245, 142, 13,
            102, 48, 227, 153, 145, 218, 70,
        ]),
        28 => VecDeque::from([
            168, 223, 200, 104, 224, 234, 108, 180, 110, 190, 195, 147, 205, 27, 232, 201, 21, 43,
            245, 87, 42, 195, 212, 119, 242, 37, 9, 123,
        ]),
        30 => VecDeque::from([
            41, 173, 145, 152, 216, 31, 179, 182, 50, 48, 110, 86, 239, 96, 222, 125, 42, 173, 226,
            193, 224, 130, 156, 37, 251, 216, 238, 40, 192, 180,
        ]),
        _ => VecDeque::new(),
    }
}

// Use the Reed-Solomon algorithm to generate error correction codes
// for our encoded data. The Reed Solomon algorithm generates a bunch
// of extra redundant data which can be used to recover the original
// data even if parts of it are missing or corrupted
fn generate_error_correction_codes(
    data: &Vec<u8>,
    level: ErrorCorrection,
    version: usize,
) -> Vec<u8> {
    let ecc_count = get_error_codeword_count(version, level);
    let generator_polynomial = get_generator_polynomial(ecc_count);

    // The coefficients of the message polynomial correspond to the data bytes
    let mut message_polynomial = data.clone();

    // Pad the message polynomial with the default values for
    // the error correction codewords
    message_polynomial.extend(iter::repeat(0).take(ecc_count));

    // Divide the message polynomial by the generator polynomial
    // The remainder will be our error correction codewords
    for i in 0..data.len() {
        let a = message_polynomial[i];
        let b = tables::GALOIS_VALUES[a as usize];
        for j in 0..generator_polynomial.len() {
            let d = (generator_polynomial[j] as u16 + b as u16) % 255;
            let value = tables::GALOIS_EXPONENTS[d as usize];
            message_polynomial[i + j + 1] ^= value;
        }
    }

    // Return the remainder
    message_polynomial[data.len()..].to_vec()
}

pub fn assemble_qr_data(input: &str, level: ErrorCorrection) -> Vec<u8> {
    let data = encode_data(input, level);
    let mode = get_encoding_mode(input);
    let version = get_version(level, mode, input.len());

    let info = tables::ECC_DATA[version - 1][level as usize];
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
            let codes = generate_error_correction_codes(&block, level, version);
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

    use crate::encoder::*;

    #[test]
    fn test_data_analyzing() {
        assert_eq!(get_encoding_mode("Hello world!"), EncodingMode::Byte);
        assert_eq!(
            get_encoding_mode("HELLO WORLD 123 :/"),
            EncodingMode::Alphanumeric
        );
        assert_eq!(get_encoding_mode("09865456789"), EncodingMode::Numeric);
        assert_eq!(get_encoding_mode("aЉ윇😱"), EncodingMode::Byte);
        assert_eq!(get_encoding_mode(""), EncodingMode::Byte);
    }

    fn test_encoding<T: bitstream_io::Numeric>(
        mode: EncodingMode,
        values: &[&str],
        expecteds: &[T],
        expected_lengths: &[u32],
    ) {
        for i in 0..values.len() {
            let encoded = match mode {
                EncodingMode::Alphanumeric => alphanumeric_encode(values[i]),
                EncodingMode::Numeric => numeric_encode(values[i]),
                EncodingMode::Byte => byte_encode(values[i]),
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
        test_encoding(EncodingMode::Numeric, &values, &bits, &lengths)
    }

    #[test]
    fn test_alphanumeric_encoding() {
        let values = ["HELLO WORLD", " $%*+-./:1"];
        let bits = [
            0b0110000101101111000110100010111001011011100010011010100001101u64,
            0b1100111100111011010101111001100011111000110111110111101u64,
        ];
        let lengths = [61, 55];
        test_encoding(EncodingMode::Alphanumeric, &values, &bits, &lengths)
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
        test_encoding(EncodingMode::Byte, &values, &bits, &lengths)
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
            0x10, 0x0C, 0x7B, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11,
            0xEC, 0x11, 0xEC, 0x11, 0xEC,
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("aЉ윇😱", ErrorCorrection::Medium);
        let expected = [
            0x40, 0xA6, 0x1D, 0x08, 0x9E, 0xC9, 0xC8, 0x7F, 0x09, 0xF9, 0x8B, 0x10, 0xEC, 0x11,
            0xEC, 0x11,
        ];
        assert_eq!(bytes, expected);

        let bytes = encode_data("", ErrorCorrection::High);
        let expected = [0x40, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC];
        assert_eq!(bytes, expected);

        let bytes = encode_data("LOREM IPSUM SIT DOLOR AMED", ErrorCorrection::Quartile);
        let expected = [
            0x20, 0xD3, 0xC9, 0x99, 0xB0, 0x09, 0xA1, 0xD0, 0xA8, 0x05, 0x3F, 0xA9, 0xEA, 0x61,
            0x79, 0x33, 0x8C, 0xEC, 0x28, 0x30, 0xEC, 0x11,
        ];
        assert_eq!(bytes, expected);
    }

    #[test]
    fn test_error_correction_coding() {
        let level = ErrorCorrection::Low;
        let version = 1;
        let bytes = vec![
            0x40, 0x56, 0x86, 0x56, 0xC6, 0xC6, 0xF0, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC,
            0x11, 0xEC, 0x11, 0xEC, 0x11,
        ];
        let correction_codes = generate_error_correction_codes(&bytes, level, version);
        let expected = [0x25, 0x19, 0xD0, 0xD2, 0x68, 0x59, 0x39];
        assert_eq!(correction_codes, expected);

        let level = ErrorCorrection::Medium;
        let bytes = vec![0x10, 0x0C, 0x7B, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11];
        let correction_codes = generate_error_correction_codes(&bytes, level, version);
        let expected = [0x1C, 0x53, 0xB9, 0x9F, 0x2B, 0xD5, 0xE3, 0x6D, 0x0E, 0x70];
        assert_eq!(correction_codes, expected);
    }

    #[test]
    fn test_data_assembly() {
        let data = assemble_qr_data("hello", ErrorCorrection::Low);
        let expected = [0x40, 0x56, 0x86, 0x56, 0xC6, 0xC6, 0xF0,
                                   0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC,
                                   0x11, 0xEC, 0x11, 0xEC, 0x11, 0x25, 0x19, 0xD0,
                                   0xD2, 0x68, 0x59, 0x39];
        assert_eq!(data, expected);

        let data = assemble_qr_data("LOREM IPSUM SIT DOLOR AMED", ErrorCorrection::High);
        let expected = [0x20, 0x61, 0xD3, 0x79, 0xC9, 0x33, 0x99, 0x8C, 0xB0, 0xEC,
                        0x09, 0x28, 0xA1, 0x30, 0xD0, 0xEC, 0xA8, 0x11, 0x05, 0xEC,
                        0x3F, 0x11, 0xA9, 0xEC, 0xEA, 0x11, 0x98, 0x12, 0x7A, 0x31,
                        0x0E, 0x41, 0x36, 0x26, 0x5B, 0xDF, 0x43, 0x34, 0x0C, 0x33,
                        0x00, 0x8B, 0x32, 0xDB, 0x28, 0x54, 0x3F, 0xE1, 0x62, 0x6C,
                        0xDF, 0x3B, 0x08, 0x65, 0x12, 0xD0, 0x35, 0xF8, 0xF0, 0x75,
                        0x1A, 0x77, 0x6D, 0x49, 0x01, 0x89, 0xB1, 0x79, 0xF4, 0x76];
        assert_eq!(data, expected);

        let data = assemble_qr_data("aЉ윇😱", ErrorCorrection::Medium);
        let expected = [0x40, 0xA6, 0x1D, 0x08, 0x9E, 0xC9, 0xC8, 0x7F, 0x09, 0xF9,
                        0x8B, 0x10, 0xEC, 0x11, 0xEC, 0x11, 0xBB, 0x3A, 0x1D, 0x62,
                        0x99, 0x9D, 0xD8, 0xFF, 0xA9, 0x0C];
        assert_eq!(data, expected);
    }
}
