QR code generation from scratch in C++

https://www.thonky.com/qr-code-tutorial
https://zavier-henry.medium.com/an-introductory-walkthrough-for-encoding-qr-codes-5a33e1e882b5
https://ntrs.nasa.gov/api/citations/19900019023/downloads/19900019023.pdf
https://tomverbeure.github.io/2022/08/07/Reed-Solomon.html
https://pythonhosted.org/PyQRCode/tables.html#pyqrcode.tables.eccwbi
https://innovation.vivint.com/introduction-to-reed-solomon-bc264d0794f8
https://en.wikiversity.org/wiki/Reed%E2%80%93Solomon_codes_for_coders

Step 1 - Data analysis:
- figure out which encoding scheme to use
    - Numeric mode: decimal digits 0-9
    - Alphanumeric mode: decimal digits 0-9, uppercase letters `$, %, *, +, -, ., /, :, as well as a space`.
    - Byte mode: Latin-1 or UTF-8 characters
    - Kanji mode: double byte characters from the Shift JIS character set
    - Extended Channel Interpretation (ECI) mode specifies the character set directly
- https://www.thonky.com/qr-code-tutorial/data-analysis
- You can also mix modes

Step 2 - Data encoding:
- https://www.thonky.com/qr-code-tutorial/data-encoding
- https://www.thonky.com/qr-code-tutorial/numeric-mode-encoding
- https://www.thonky.com/qr-code-tutorial/alphanumeric-mode-encoding
- https://www.thonky.com/qr-code-tutorial/byte-mode-encoding
- https://www.thonky.com/qr-code-tutorial/kanji-mode-encoding
- https://www.thonky.com/qr-code-tutorial/error-correction-table
- encode the text given the encoding mode
- split resulting bits into byte "codewords"

Step 3 - Error Correction Coding:
- Use Reed Solomon error correction to generated error correcting codewords
- Low (L, 7%), Medium (M, 15%), Quantile (Q, 25%), High (H, 30%) error correction (recovering data)
- Galois field (GF) -- finite set of number with operations resulting in numbers within the same set
  Basically, you perform regular arithmetic then modulo the result at the end
- QR spec uses modulo 2 bitwise arithmetic (XOR) and modulo 285 byte wise arithmetic
  So, it uses a Galois Field 2^8 (GF(256)) (numbers between 0 and 255)
- In the Galois Field, every number can be represented as 2 ^ n, but 2 ^ n when evaluated is also within the field (0 - 255)
- Addition and Subtraction in the galois field are the same thing, simply do the operation, then modulo by 285
- Multiplication is just adding the exponents since as we know all numbers in the galois field can be represented as 2 ^ n
- The powers of 2 build on each other since 2 ^ n is the same as 2 * 2 ^ n - 1, but if that results in something bigger than
  255, then restart the sort of cycle by xoring by 285
- You could represent the coefficients of a polynomial in an array, since the exponents always follow then same decrecending pattern

Step 4 - Structure Final Message:
- structure data and error correction codewords

Step 5 - Module Placement in Matrix:
- arrange the data in the qr matrix

Step 6 - Data masking:
- determine the best mask to use and use that to mask the matrix

Step 7 - Format and Version Information:
- add version and format info to the matrix
- Verion 1 (21x21 pixels), Version 2 (25x25), to Version 40 (177x177 pixels)
