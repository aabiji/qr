#![allow(unused)]

mod encoder;
mod tables;
mod drawer;

use image::{ImageBuffer, Rgb};

fn main() {
    let mut img = ImageBuffer::new(400, 400);

    for (x, y, pixel) in img.enumerate_pixels_mut() {
        let r = 255 as u8;
        let g = 255 as u8;
        let b = 255 as u8;
        *pixel = Rgb([r, g, b]);
    }

    img.save("target/qrcode.jpg");
}
