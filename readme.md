QR code generation from scratch in C++
[Reference](https://www.thonky.com/qr-code-tutorial)
https://zavier-henry.medium.com/an-introductory-walkthrough-for-encoding-qr-codes-5a33e1e882b5

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

Step 4 - Structure Final Message:
- structure data and error correction codewords

Step 5 - Module Placement in Matrix:
- arrange the data in the qr matrix

Step 6 - Data masking:
- determine the best mask to use and use that to mask the matrix

Step 7 - Format and Version Information:
- add version and format info to the matrix
- Verion 1 (21x21 pixels), Version 2 (25x25), to Version 40 (177x177 pixels)
