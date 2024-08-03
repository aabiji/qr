use std::io::Write;

fn get_input(prompt: &str) -> String {
    print!("{}", prompt);
    std::io::stdout().flush().unwrap();
    let mut line = String::new();
    std::io::stdin().read_line(&mut line).unwrap();
    line.trim().to_string()
}

fn main() {
    println!("QR Code Generator v0.1");
    println!("Possible error correction levels are: (L)ow, (M)edium, (Q)uartile, and (H)igh");

    let input = get_input("Input > ");
    let output = get_input("Output file path > ");
    let level = get_input("Error correction level > ");

    let error_correction = match level.trim() {
        "L" => qrgen::ErrorCorrection::Low,
        "M" => qrgen::ErrorCorrection::Medium,
        "Q" => qrgen::ErrorCorrection::Quartile,
        "H" => qrgen::ErrorCorrection::High,
        _ => qrgen::ErrorCorrection::Low,
    };
    qrgen::generate_qr_code(input.as_str(), error_correction, output.as_str());
}
