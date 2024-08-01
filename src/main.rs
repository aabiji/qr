#![allow(unused)]

mod drawer;
mod encoder;
mod tables;

fn main() {
    let input = "Lorem ipsum sit dolor amed.Lorem ipsum sit dolor amed.Lorem ipsum sit dolor amed.";
    let level = encoder::ErrorCorrection::High;
    let qr = drawer::QR::create(input, level);

    let pixel_size = 12;
    let size = qr.size as u32 * pixel_size;
    let mut img = image::ImageBuffer::new(size, size);

    for (x, y, pixel) in img.enumerate_pixels_mut() {
        let matrix_x = x / pixel_size;
        let matrix_y = y / pixel_size;
        let index = (matrix_y * qr.size as u32 + matrix_x) as usize;
        let color = qr.matrix[index];
        *pixel = image::Rgb([color, color, color]);
    }

    img.save("target/qrcode.png");
}
