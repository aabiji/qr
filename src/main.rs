/// Building a qr code generator:
/// First step:
/// 1. Analyze data to determine the optimal data encoding mode

#[derive(Debug, PartialEq)]
enum EncodingMode {
    Numeric,
    Alphanumeric,
    Byte,
}

fn is_alphanumeric(codepoint: char) -> bool {
    let num_range = '0'..'9';
    let char_range = 'A'..'Z';
    let special = match codepoint {
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
    };
    num_range.contains(&codepoint) || char_range.contains(&codepoint) || special
}

fn determine_encoding_mode(input: &str) -> EncodingMode {
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

#[cfg(test)]
mod test {
    use crate::*;

    #[test]
    fn test_data_analyzing() {
        assert_eq!(determine_encoding_mode("Hello world!"), EncodingMode::Byte);
        assert_eq!(
            determine_encoding_mode("HELLO WORLD 123 :/"),
            EncodingMode::Alphanumeric
        );
        assert_eq!(
            determine_encoding_mode("09865456789"),
            EncodingMode::Numeric
        );
        assert_eq!(determine_encoding_mode("aЉ윇😱"), EncodingMode::Byte);
    }
}

fn main() {}
