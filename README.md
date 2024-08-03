A qr generator writting in Rust. QR codes encode data into a image that can be scanned.
The generator works and is also well tested. This crate is split into library (`qrgen`)
and binary (`qr`) modules. To run, simply use `cargo run`.

Generation steps:
1. Determine which encoding mode to use
2. Encode the data
3. Generate error correction codewords
4. Interleave blocks if necessary
5. Place the data and error correction bits in the matrix
6. Apply the mask patterns and determine which one results in the lowest penalty
7. Add format and version information

![Example QR code](qrcode.png)

Docs:
- [Creating a qr code step by step](https://www.nayuki.io/page/creating-a-qr-code-step-by-step)
- [Data tables](https://pythonhosted.org/PyQRCode/tables.html)
- [Explanation of the Reed Solomon algorithm](https://en.wikiversity.org/wiki/Reed%E2%80%93Solomon_codes_for_coders)
- [QR tutorial](https://www.thonky.com/qr-code-tutorial)
