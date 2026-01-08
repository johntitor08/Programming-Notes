# Rust Programlama

## Giriş
Rust, güvenlik, hız ve eşzamanlılık üzerine odaklanmış modern bir sistem programlama dilidir. Mozilla tarafından geliştirilmiş ve özellikle bellek güvenliği konusunda öne çıkmıştır.

---

## Özellikler
- Bellek güvenliği (garbage collector olmadan)
- Yüksek performans
- Eşzamanlı programlamada güvenlik
- Güçlü tip sistemi

---

## Kurulum
Rust kurulumu için aşağıdaki adımlar takip edilir:
```bash
# Rustup (önerilen yöntem)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```
Kurulumdan sonra:
```bash
rustc --version
```

---

## İlk Program
```rust
fn main() {
    println!("Merhaba, Rust!");
}
```
Derlemek ve çalıştırmak için:
```bash
rustc main.rs
./main
```

---

## Değişkenler
```rust
fn main() {
    let x = 5;
    println!("x: {}", x);

    let mut y = 10;
    y = 20;
    println!("y: {}", y);
}
```

---

## Veri Tipleri
- **tamsayılar**: i8, i16, i32, i64, i128
- **ondalıklı sayılar**: f32, f64
- **boolean**: bool
- **karakter**: char

---

## Kontrol Yapıları
```rust
fn main() {
    let sayi = 7;
    if sayi < 10 {
        println!("10'dan küçük");
    } else {
        println!("10 veya daha büyük");
    }
}
```

---

## Döngüler
```rust
fn main() {
    for i in 0..5 {
        println!("i: {}", i);
    }
}
```

---

## Fonksiyonlar
```rust
fn topla(a: i32, b: i32) -> i32 {
    a + b
}

fn main() {
    let sonuc = topla(5, 7);
    println!("Sonuç: {}", sonuc);
}
```

---

## Yapılar (Struct)
```rust
struct Kisi {
    isim: String,
    yas: u8,
}

fn main() {
    let kisi = Kisi {
        isim: String::from("Ahmet"),
        yas: 30,
    };

    println!("{} - {} yaşında", kisi.isim, kisi.yas);
}
```

---

## Enum
```rust
enum Renk {
    Kirmizi,
    Yesil,
    Mavi,
}

fn main() {
    let r = Renk::Kirmizi;
    match r {
        Renk::Kirmizi => println!("Kırmızı seçildi"),
        Renk::Yesil => println!("Yeşil seçildi"),
        Renk::Mavi => println!("Mavi seçildi"),
    }
}
```

---

## Koleksiyonlar
### Vektör
```rust
fn main() {
    let mut sayilar = vec![1, 2, 3];
    sayilar.push(4);
    println!("{:?}", sayilar);
}
```

### HashMap
```rust
use std::collections::HashMap;

fn main() {
    let mut sozluk = HashMap::new();
    sozluk.insert("elma", 3);
    sozluk.insert("armut", 5);

    println!("{:?}", sozluk);
}
```

---

## Borrowing ve Ownership
```rust
fn main() {
    let s1 = String::from("Merhaba");
    let s2 = &s1; // borrowing
    println!("{} ve {}", s1, s2);
}
```

---

## Traitler
Traitler, Rust'ta arayüzlere (interface) benzer. Bir yapının hangi fonksiyonları uygulaması gerektiğini tanımlar.
```rust
trait Ses {
    fn ses_cikar(&self);
}

struct Kedi;

impl Ses for Kedi {
    fn ses_cikar(&self) {
        println!("Miyav!");
    }
}

fn main() {
    let k = Kedi;
    k.ses_cikar();
}
```

---

## Error Handling
Rust'ta hata yönetimi `Result` ve `Option` tipleri üzerinden yapılır.
```rust
fn bolme(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        Err(String::from("Sıfıra bölme hatası"))
    } else {
        Ok(a / b)
    }
}

fn main() {
    match bolme(10, 2) {
        Ok(sonuc) => println!("Sonuç: {}", sonuc),
        Err(hata) => println!("Hata: {}", hata),
    }
}
```

---

## Asenkron Programlama (async/await)
```rust
use tokio::time::{sleep, Duration};

async fn bekle() {
    println!("Bekleniyor...");
    sleep(Duration::from_secs(2)).await;
    println!("Bitti!");
}

#[tokio::main]
async fn main() {
    bekle().await;
}
```

---

## Paket Yönetimi (Cargo)
Yeni proje oluşturma:
```bash
cargo new proje_adi
```
Derleme ve çalıştırma:
```bash
cargo run
```
Bağımlılık eklemek için `Cargo.toml` dosyasında:
```toml
[dependencies]
rand = "0.8"
```

---

## Sonuç
Rust, güvenli bellek yönetimi, yüksek performans ve modern özellikleriyle sistem programlama için güçlü bir dildir. Traitler, hata yönetimi, async/await ve Cargo sayesinde hem sistem hem de uygulama geliştirmede geniş bir ekosistem sunar.

