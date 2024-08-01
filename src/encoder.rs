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
pub enum EncodingMode {
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

pub fn get_encoding_mode(input: &str) -> EncodingMode {
    let mut num_count = 0;
    let mut alpha_count = 0;

    for codepoint in input.chars() {
        if codepoint.is_ascii_digit() {
            num_count += 1;
        }
        if is_alphanumeric(codepoint) {
            alpha_count += 1;
        }
    }

    if input.len() == 0 {
        return EncodingMode::Byte;
    } else if num_count == input.len() {
        return EncodingMode::Numeric;
    } else if alpha_count == input.len() {
        return EncodingMode::Alphanumeric;
    }

    EncodingMode::Byte
}

// Get the minimum qr version that can hold the data
pub fn get_version(level: ErrorCorrection, mode: EncodingMode, num_chars: usize) -> usize {
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

// Use the Reed-Solomon algorithm to generate error correction codes
// for our encoded data. The Reed Solomon algorithm generates a bunch
// of extra redundant data which can be used to recover the original
// data even if parts of it are missing or corrupted
fn generate_error_correction_codes(
    data: &Vec<u8>,
    level: ErrorCorrection,
    version: usize,
) -> Vec<u8> {
    let ecc_count = tables::ECC_DATA[version - 1][level as usize][0] as usize;
    let generator_polynomial = tables::get_generator_polynomial(ecc_count);

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

/// Encode data, generate error correction codes and interleave to get the final qr data
pub fn assemble_qr_data(input: &str, level: ErrorCorrection) -> Vec<u8> {
    let data = encode_data(input, level);
    let mode = get_encoding_mode(input);
    let version = get_version(level, mode, input.len());

    let info = tables::ECC_DATA[version - 1][level as usize];
    let ecc_count = info[0] as usize;
    let block_counts = [info[1] as usize, info[3] as usize];
    let block_lengths = [info[2] as usize, info[4] as usize];

    let mut error_codewords = Vec::new();
    let mut interleaved = Vec::new();

    // FIXME: this indexing scheme is wrong --> 0xF7 instead of 0xC6
    // Generate error correction codewords for each block
    // and interleave the data codewords
    let max_length = std::cmp::max(block_lengths[0], block_lengths[1]);
    for i in 0..max_length {
        for group in 0..2 {
            for j in 0..block_counts[group] {
                let group_length = if group == 0 {
                    0
                } else {
                    block_counts[0] * block_lengths[0] // Continue on from the first group
                };

                let index = group * group_length + j * block_lengths[group] + i;
                if i >= block_lengths[group] {
                    continue;
                } else if i == 0 {
                    let block = data[index..index + block_lengths[group]].to_vec();
                    error_codewords.extend(generate_error_correction_codes(&block, level, version));
                }
                println!("{:X}", data[index]);
                interleaved.push(data[index]);
            }
        }
    }

    // Interleave the error correction codewords
    for i in 0..ecc_count {
        for group in 0..2 {
            for j in 0..block_counts[group] {
                let group_length = block_counts[group] * ecc_count;
                let index = group * group_length + j * ecc_count + i;
                interleaved.push(error_codewords[index]);
            }
        }
    }

    interleaved
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
        let bytes = vec![
            0x10, 0x0C, 0x7B, 0x00, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11,
            0xEC, 0x11,
        ];
        let correction_codes = generate_error_correction_codes(&bytes, level, version);
        let expected = [0x1C, 0x53, 0xB9, 0x9F, 0x2B, 0xD5, 0xE3, 0x6D, 0x0E, 0x70];
        assert_eq!(correction_codes, expected);
    }

    #[test]
    fn test_data_assembly() {
        /*
        let data = assemble_qr_data("hello", ErrorCorrection::Low);
        let expected = [
            0x40, 0x56, 0x86, 0x56, 0xC6, 0xC6, 0xF0, 0xEC, 0x11, 0xEC, 0x11, 0xEC, 0x11, 0xEC,
            0x11, 0xEC, 0x11, 0xEC, 0x11, 0x25, 0x19, 0xD0, 0xD2, 0x68, 0x59, 0x39,
        ];
        assert_eq!(data, expected);

        let data = assemble_qr_data("LOREM IPSUM SIT DOLOR AMED", ErrorCorrection::High);
        let expected = [
            0x20, 0x61, 0xD3, 0x79, 0xC9, 0x33, 0x99, 0x8C, 0xB0, 0xEC, 0x09, 0x28, 0xA1, 0x30,
            0xD0, 0xEC, 0xA8, 0x11, 0x05, 0xEC, 0x3F, 0x11, 0xA9, 0xEC, 0xEA, 0x11, 0x98, 0x12,
            0x7A, 0x31, 0x0E, 0x41, 0x36, 0x26, 0x5B, 0xDF, 0x43, 0x34, 0x0C, 0x33, 0x00, 0x8B,
            0x32, 0xDB, 0x28, 0x54, 0x3F, 0xE1, 0x62, 0x6C, 0xDF, 0x3B, 0x08, 0x65, 0x12, 0xD0,
            0x35, 0xF8, 0xF0, 0x75, 0x1A, 0x77, 0x6D, 0x49, 0x01, 0x89, 0xB1, 0x79, 0xF4, 0x76,
        ];
        assert_eq!(data, expected);

        let data = assemble_qr_data("aЉ윇😱", ErrorCorrection::Medium);
        let expected = [
            0x40, 0xA6, 0x1D, 0x08, 0x9E, 0xC9, 0xC8, 0x7F, 0x09, 0xF9, 0x8B, 0x10, 0xEC, 0x11,
            0xEC, 0x11, 0xBB, 0x3A, 0x1D, 0x62, 0x99, 0x9D, 0xD8, 0xFF, 0xA9, 0x0C,
        ];
        assert_eq!(data, expected);

        let input: String = iter::repeat("Hello, world! 123").take(5).collect();
        let data = assemble_qr_data(input.as_str(), ErrorCorrection::High);
        let expected = [
            0x45, 0xC6, 0xC2, 0x86, 0x12, 0xF7, 0xF2, 0xEC, 0x54, 0x42, 0x07, 0x56, 0x03, 0x26,
            0xC2, 0x11, 0x86, 0x12, 0x76, 0xC6, 0x13, 0xC6, 0x07, 0xEC, 0x56, 0x03, 0xF7, 0xC6,
            0x23, 0x42, 0x76, 0x11, 0xC6, 0x13, 0x26, 0xF2, 0x34, 0x12, 0xF7, 0xEC, 0xC6, 0x23,
            0xC6, 0xC2, 0x86, 0x03, 0x26, 0x11, 0xF2, 0x34, 0x42, 0x07, 0x56, 0x13, 0xC6, 0xEC,
            0xC2, 0x86, 0x12, 0x76, 0xC6, 0x23, 0x42, 0x11, 0x07, 0x56, 0x03, 0xF7, 0xC6, 0x34,
            0x12, 0xEC, 0x76, 0xC6, 0x13, 0x26, 0xF2, 0x86, 0x03, 0x11, 0xF7, 0xC6, 0x23, 0xC6,
            0xC2, 0x56, 0x13, 0xEC, 0x26, 0xF2, 0x34, 0x42, 0x07, 0xC6, 0x23, 0x11, 0x76, 0xC6,
            0x30, 0xEC, 0xC0, 0x58, 0xD8, 0xAC, 0x35, 0x66, 0x0E, 0xE2, 0x2E, 0x8B, 0x3A, 0xAF,
            0xDE, 0xED, 0x8E, 0x34, 0x6E, 0x1D, 0x4F, 0x84, 0xE2, 0x45, 0x4A, 0x07, 0xB8, 0x70,
            0xB2, 0xD9, 0x7D, 0x6C, 0xE2, 0x4D, 0xBA, 0x16, 0x68, 0x7D, 0x38, 0xDB, 0x6C, 0x1B,
            0x27, 0x7F, 0x39, 0xC4, 0x96, 0xC5, 0x45, 0xD7, 0xA1, 0xA5, 0x1C, 0x42, 0x68, 0x33,
            0x38, 0x76, 0xC7, 0x3F, 0xA9, 0x62, 0xED, 0xFD, 0xCF, 0xFA, 0x35, 0xB4, 0xCB, 0x32,
            0xAE, 0x1E, 0xCB, 0xBC, 0xA5, 0xA5, 0x83, 0x2F, 0xB0, 0x8E, 0xFE, 0xD3, 0x07, 0x7C,
            0xF8, 0xE4, 0xDE, 0x81, 0x2C, 0x4B, 0x79, 0x8F, 0x9E, 0x01, 0x20, 0x0E, 0xDC, 0x45,
            0x82, 0xC1, 0xD9, 0x89, 0x53, 0x48, 0x89, 0x56, 0x06, 0x91, 0x67, 0x6A, 0x9F, 0x48,
            0xF2, 0x88, 0x31, 0xA7, 0x4C, 0x3E, 0x2F, 0x40, 0x34, 0x38, 0xDC, 0xC1, 0x6B, 0xA8,
            0xA0, 0xA9, 0x19, 0x1E, 0x4D, 0x23, 0x35, 0xA7, 0x70, 0xA6, 0x67, 0xF5, 0xD3, 0xBF,
            0x53, 0x70, 0x86, 0xFA, 0x36, 0x24, 0xE2, 0x77, 0x0A, 0xB4, 0xC2, 0x80, 0xC4, 0xAB,
            0x6E, 0x76, 0xA4, 0xA1, 0x3C, 0x8B, 0x03, 0x77, 0xB1, 0xA1, 0x19, 0x81, 0xD7, 0x99,
            0x82, 0x79, 0xA0, 0xD8, 0xDA, 0x46, 0x5C, 0xF4, 0xE6, 0x5D, 0x7A, 0xED, 0xB8, 0x60,
            0x18, 0x36, 0xDA, 0xA0, 0x6C, 0x23, 0x0C, 0xEB, 0xE3, 0x18, 0x48, 0x20,
        ];
        assert_eq!(data, expected);
        */

        let input =
            "Lorem ipsum sit dolor amed.Lorem ipsum sit dolor amed.Lorem ipsum sit dolor amed.";
        let data = assemble_qr_data(input, ErrorCorrection::High);
        let expected = [
            0x45, 0x36, 0xE4, 0x97, 0xC6, 0x06, 0x14, 0x97, 0xC6, 0x42, 0xF7, 0x46, 0xC6, 0x42,
            0xF7, 0x06, 0x26, 0xF6, 0xF7, 0x06, 0x26, 0x46, 0x56, 0xC6, 0x26, 0x46, 0x56, 0xF6,
            0xD2, 0xF7, 0x56, 0xF6, 0xD2, 0xC6, 0x06, 0x22, 0xD2, 0xC6, 0x06, 0xF7, 0x97, 0x06,
            0x06, 0xF7, 0x97, 0x22, 0x07, 0x16, 0x97, 0x22, 0x07, 0x06, 0x37, 0xD6, 0x07, 0x06,
            0x37, 0x16, 0x56, 0x56, 0x37, 0x16, 0x56, 0xD6, 0xD2, 0x42, 0x56, 0xD6, 0xD2, 0x56,
            0x07, 0xE0, 0xD2, 0x56, 0x07, 0x42, 0x36, 0xEC, 0x07, 0x42, 0x36, 0xE4, 0x97, 0x11,
            0x42, 0xEC, 0x3F, 0xD2, 0x9E, 0x10, 0x09, 0x23, 0xE5, 0xF9, 0xC9, 0x1D, 0xBE, 0xA1,
            0x00, 0x74, 0x68, 0xCF, 0xC8, 0xD2, 0x65, 0x1E, 0xF7, 0x48, 0x57, 0xAF, 0xBE, 0x8E,
            0xDA, 0x88, 0x1F, 0xE9, 0xAC, 0xF7, 0xA8, 0x37, 0x20, 0xC0, 0x24, 0x48, 0x31, 0xC0,
            0x80, 0x19, 0xC7, 0x0A, 0x8D, 0x4F, 0xC5, 0x5C, 0x12, 0x6F, 0x81, 0x2A, 0x6A, 0x45,
            0x66, 0x70, 0x0B, 0x84, 0x70, 0xDB, 0x8E, 0x82, 0x81, 0xF8, 0x85, 0x6D, 0x71, 0x30,
            0x89, 0x50, 0xB4, 0xDF, 0xBE, 0x09, 0x14, 0x39, 0xD4, 0xAD, 0xB6, 0xD9, 0x9C, 0xE1,
            0x61, 0xC3, 0x8B, 0xB5, 0xED, 0x08, 0xB9, 0xDB, 0xC3, 0xEA, 0x69, 0x42, 0x43, 0x30,
            0xB6, 0xB6, 0xF3, 0x73, 0x84, 0xA4, 0xEC, 0xF5, 0xC8, 0x5E, 0x47, 0xC9, 0x8E, 0x1B,
            0xA8, 0x3E, 0x6D, 0x3C, 0x98, 0x32, 0x47, 0x04, 0x95, 0x0E, 0x42, 0x57, 0x9F, 0xEF,
            0xAF, 0x02, 0x9E, 0x76, 0x8B, 0xDC, 0x07, 0xDA, 0x1E, 0x51, 0x84, 0x10, 0xF4, 0x55,
            0xF8, 0x2E, 0xE1, 0x65, 0x31, 0x63, 0xC0, 0xD6, 0x05, 0x32, 0xB6, 0x00, 0xD3, 0x2E,
            0x4B, 0x0B, 0x84, 0x0D,
        ];
        assert_eq!(data, expected);
    }
}
