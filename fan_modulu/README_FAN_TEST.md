# ESP32 - Fan Motoru Test Modülü (L298N)

Bu proje,  **L298N Motor Sürücü ve Fan** entegrasyonunu bağımsız olarak test etmek için oluşturulmuştur (`fan_modulu/`).

Ana projeye eklemeden önce yazılım ve donanım bağlantılarının doğruluğunu bu mini proje üzerinden test edebiliriz.

---

## 🔌 1. Donanım Bağlantı Şeması

L298N motor sürücü modülü ile ESP32 arasındaki bağlantılar aşağıdaki gibi yapılmalıdır:

| L298N Pini      | ESP32 Pini / Kaynak | Açıklama                                                                                                                                                                |
|:--------------- |:------------------- |:----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **IN4**         | **GPIO 18**         | Fanı açıp kapatmak için tetik sinyali (3.3V yeterlidir).                                                                                                                |
| **IN3**         | **GND**             | Fan sadece tek yönde döneceği için (üfleme) bu pini toprağa bağlıyoruz.                                                                                                 |
| **ENB**         | **5V (Köprü)**      | L298N üzerindeki ENB pininde genellikle küçük siyah bir "jumper (köprü)" takılıdır. Bu jumper kalsın, fan tam hızda (PWM olmadan) çalışacaktır.                         |
| **GND**         | **ESP32 GND**       | ⚠️ **ZORUNLU!** ESP32'nin herhangi bir GND pini ile L298N üzerindeki GND klemensi mutlaka birbirine bağlanmalıdır. Aksi takdirde sinyal voltajları uyuşmaz ve çalışmaz. |
| **GND**         | **Pil (-)**         | Güç kaynağının eksi kutbu.                                                                                                                                              |
| **12V / VCC**   | **Pil (+)**         | Güç kaynağının artı kutbu (5V - 12V arası pil bloğu).                                                                                                                   |
| **OUT3 & OUT4** | **Fan Motoru**      | Fanın iki kablosu buraya bağlanır. Yön ters devredeyse kabloların yerini değiştirebilirsiniz.                                                                           |

---

## 🧠 2. Çalışma Mantığı ve Özellikler

Bu modül iki farklı konsepti aynı anda yönetmek için tasarlandı:

1. **Otomatik (Sıcaklık) Modu:** Sensör (DS18B20) verisi `28°C`'yi aşarsa fan otomatik çalışır, altına düşerse kapanır.
2. **Manuel (Sesli Komut) Modu:** Kullanıcı *"Fanı kapat"* veya *"Fanı aç"* derse (`FAN_OFF` / `FAN_ON` aksiyonları).

**Çatışma (Conflict) Çözümü - Manuel Override:**
Eğer hava çok sıcaksa (örneğin 32°C) fan otomatik çalışacaktır. Ancak kullanıcı sesten rahatsız olup *"Fanı kapat"* derse, yazdığımız **Manuel Override** sistemi devreye girer. Bu sistem:

- Sensör verisini "belli bir süre" (bu test projesinde 30 saniye olarak simüle edildi) görmezden gelir.
- Böylece akıllı asistan, kullanıcının komutuna itaat eder ve sıcaklığa rağmen fanı açmaz.
- Süre dolduğunda sıcaklık halen yüksekse, otomasyon tekrar kontrolü ele alır ve fan çalışır.

---

## 🧪 3. Test Senaryosu (`main.c`)

Bu test kodunda fiziksel bir sıcaklık sensörü beklemiyoruz. Kod, zaman çizelgesine göre sanal sıcaklık ve hayali konuşma komutları üreterek sistemi test eder.

Projeyi flashlayıp Serial Monitor'u açtığınızda arka arkaya loglar göreceksiniz. Kodun test ettiği kronolojik akış şudur:

1. **[0 - 30. saniye]:** Sistem sıcaklığı 25°C okuyor (Eşik 28°C). Fan **KAPALI** bekliyor.
2. **[30 - 60. saniye]:** Sistem sıcaklığı 32°C okuyor. Eşik aşıldığı için fan **OTOMATİK AÇILIYOR**.
3. **[60. saniye]:** Kullanıcı mikrofondan *"Fanı kapat"* komutu göndermiş gibi `FAN_OFF` tetiklenir. Override başlar ve fan **KAPANIR**.
4. **[61 - 90. saniye]:** Sıcaklık halen 32°C olmasına rağmen Override devrede olduğu için fan inatla **KAPALI KALIR**.
5. **[90. saniye]:** 30 saniyelik Override süresi dolar. Sistem otomatik moda döner ve 32°C sıcaklık yüzünden fan **TEKRAR AÇILIR**.
6. **[120. saniye]:** Kullanıcı mikrofondan *"Fanı çalıştır"* komutu göndermiş gibi `FAN_ON` tetiklenir. Override ile manuel olarak fan çalışmaya ayarlanır.
7. **[150. saniye]:** Odadaki sıcaklık aniden 20°C'ye düşer (Sanal).
8. **[150. saniye sonrası]:** Override bittiğinde sıcaklık eşiğin (28°C) altında olduğu için sistem fanı tamamen **KAPATIR**.



---

## 🔄 4. Ana Projeye Aktarım

Bu modüldeki testler tamamlandığında `mert` klasöründeki ana projeye entegrasyon çok basittir:

1. Bu projedeki kilit dosyalar olan `fan.h` ve `fan.c` dosyalarını `mert/main/` dizinine kopyalayın.
2. `mert/main/CMakeLists.txt` içerisine `fan.c`'yi kaynaklar dizisine ekleyin.
3. `mert/main/config.h` içerisine GPIO ve eşik tanımlarını ekleyin (`FAN_IN4_GPIO 18` gibi).
4. `mert/main/main.c` içerisine `fan_init()` ve gerekli döngü / aksiyon kodlarını dahil edin. (Bunun için sana hazırladığım `implementation_plan.md` dosyasına bakabilirsiniz).
