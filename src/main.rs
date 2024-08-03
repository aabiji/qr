#![allow(unused)]

mod drawer;
mod encoder;
mod tables;

fn main() {
    let input = "Pretty cool!";
    let level = encoder::ErrorCorrection::Low;
    let qr = drawer::QR::create(input, level);

    let pixel_size = 10;
    let outer_padding = 4 * pixel_size;
    let size = qr.size as u32 * pixel_size;
    let img_size = size + outer_padding * 2;
    let mut img = image::ImageBuffer::new(img_size, img_size);

    for (x, y, pixel) in img.enumerate_pixels_mut() {
        let x_inside = x >= outer_padding && x < size + outer_padding;
        let y_inside = y >= outer_padding && y < size + outer_padding;
        if x_inside && y_inside {
            let matrix_x = (x - outer_padding) / pixel_size;
            let matrix_y = (y - outer_padding) / pixel_size;
            let index = (matrix_y * qr.size as u32 + matrix_x) as usize;
            let color = qr.matrix[index];
            *pixel = image::Rgb([color, color, color]);
        } else {
            *pixel = image::Rgb([255, 255, 255]);
        }
    }

    img.save("target/qrcode.png");
}
