OpenGl ile Oluşturulmuş Oyun Simülasyonu

  Oyun bir kedinin zemin üzerinde bulunan piramitlere çarparak o piramitleri yıkması ve piramitler içerisinde saklanan altınları açığa çıkarması mantığına dayanır.

  Kullanılan Teknik Bileşenler ve Kütüphaneler 

Kütüphaneler: GLM, GLFW, GLEW, GLUT

Teknikler ve Özellikler
 3D Modelleme
    • Kedi, zemin, piramitler ve küpler manuel olarak vertex dizileri ile oluşturuluyor.
    • Modeller glDrawArrays ile çiziliyor.
 Kamera Modları
    • 3 adet kamera:
        ◦ First Person (1): Kedi gözünden görünüm.
        ◦ Free Mode (2): WASD + mouse ile serbest dolaşım.
        ◦ Fixed Story (3): Sabit kamerayla takip.
 Hareket Sistemi
    • W, A, S, D ile kedinin x/z düzleminde hareketi.
    • X ve Z ile bazı kamera modlarında yakınlaşma/uzaklaşma.
 Çarpışma Kontrolü
    • Kedi ile piramitlerin çarpışması kontrol ediliyor.
    • Çarpışma gerçekleştiğinde piramit devriliyor.
 Doku (Texture) Kullanımı
    • Zemin: Taş zemin görünümü ile kaplanmış bir yüzey.
    • Kedi: Kürk benzeri doku kullanılıyor.
    • Piramitler: Piramit görünümüyle kaplanıyor.
    • Texture’lar GL_TEXTURE_2D olarak yükleniyor ve glBindTexture ile etkinleştiriliyor.



Render videosu linki:  https://youtu.be/GEWkMW2tTLQ

Not: Videodaki ses sonradan eklendi, oyuna dahil değil.
