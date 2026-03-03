// Translated by Ömer AKDAŞ

// DIALOG(EX)
#define S10000	"Emulation"
#define S10001	"benzeşen Port&adresi:"
#define S10002	"LPT-&genişletme:"
#define S10003	"Yönetim metodu IN- ve OUT-komutları"
#define S10004	"hata ayıklma bildirimi aracılığı ile"
#define S10005	"(tüm tarzları IN ve OUT komutlarından oluşturur)"
#define S10006	"&kullanım oranını zorlamak, meşgul görünseler bile"
#define S10007	"Yönlerdirilmesi: &READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10008	"(daha hızlı ve daha stable)"
#define S10009	"Performans-seçenekleri"
#define S10010	"OUT komutu için önbellek"
#define S10011	"(Daha fazla OUT-komutunun bir USB-Paketine özetlenmesi)"
#define S10012	"Paket gönderimi şu komuta zorlar"
#define S10014	"Dağıtım-kayıt'ın bir kopyasının yeniden okunması"
#define S10015	"&daha doğrusu…"
#define S10016	"(son olarak IN-Komutunda yazılan değeri oluşturur; çıkışlar harici donanım tarafından (az) yönlendirilirse çalışmaz)"

#define S10100	"İstatistikler"
#define S10101	"Hata ayıklayıcı kaydı ve bidirimi"
#define S10102	"&sıfıra ayarlama"
#define S10103	"OUT-komutları:"
#define S10104	"IN-komutları:"
#define S10105	"Benzeşme hataları:"
#define S10106	"(desteklenmeyen IN- veya OUT-komutları, bspw. OUTSB)"
#define S10107	"Kaybolmuş hata ayıklayıcı kaydı"
#define S10108	"(Bilinmeyen sürücü yazılımı Hata ayıklama kaydının üzerine yazdı)"
#define S10109	"üzerine yazılan hata ayıklama kaydı bir 100-ms-zamanlayıcı tarafından belirlendi"
#define S10110	"gerçekte tahsis edilcek konum:"
#define S10114	"Yönlendirme READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10115	"Sıfıra ayarlama"
#define S10116	"Ekstra uzunluk:"
#define S10117	"USB2LPT &özel…"
#define S10118	"Firmware &güncelleme…"
#define S10119	"haftmann#software, Chemnitz Teknik Üniversitesi, Elektronik ve Bilgi Teknikleri, Ölçme ve Sensör teknikleri Profesörlüğü, 2006-2009"

#define S10200	"Dağıtım-kayıt'ın bir kopyası"
#define S10201	"&Veri portu (Basisadresse + 0)"
#define S10202	"&yönlendirme portu (Basisadresse + 2)"
#define S10203	"&Ekstra-Portlar"
#define S10204	"Tamam"
#define S10205	"İptal"

// Son sayfa donanım asistanları içindir(CoInstaller; sadece Win32)
#define S10302	"Yanlış girilmiş bir adres bilgisayarınızı kilitleyebilir veya daha da kötüsünü yapabilir! Verilmiş üç direktifin kullanımı güvenli sayılır."
#define S10304	"LPT genişletme başka port adreslerinin benzeşmesini de etkiler.\nBu nispeten nadir ve SPP gerektirir-ayarı tamam."
#define S10305	"Bu ek ayarları operasyonda cihaz yöneticisi mülkiyet sayfalarında olabilir yapılır."

#define S10400	"USB2LPT özel"
#define S10401	"Özellik Register"
#define S10402	"(Nitelik-Kaydı USB2LPT'nin EPPROM'unda süreklik saklanır. Erişim Cihaz kontrolü veya Temel ile)"
#define S10403	"Veri portları için açık &kolektörler (+0)"
#define S10404	"Yönlendirme portları için &açık portlar (+2)"
#define S10405	"&Pulluplar (ve 5V de Pin25, yeniden faal olması durumunda) açılması"
#define S10406	"&Direk takma/çıkarma modu (USB2LPT wird zum I/O-Gerät)"
#define S10407	"(Çıkış voltajını 3.3V yerine 5V yapar)"
#define S10408	"(Gri bölge şu anlama gelir: SPP-Modunda açık kolektörler, Diğer her modda açma-kapama)"
#define S10409	"(Enerji tasarruf tedbiri)"
#define S10410	"(Yatırımların kaldırılması. Bu yüzden doğru hiçbir paralel port simulasyonuna cihaz kontrolü ile erişilemez.4. kısımda 17 yerine 20 adet takma/çıkarma kumandası mevuttur.Herbir pin kontrol edilebilir.)"
#define S10411	"Yerleştirmek && &yeniden okumak"
#define S10412	"Kısa &bitirme testi (bir çevre olmadan!)"
#define S10413	"Seri numarası"
#define S10414	"&USB tanımlayıcıda"			// Google
#define S10415	"Firmware tarihi"
#define S10416	"Soluk mavi &LED"			// Google
#define S10417	"&ECR Başlangıç ​​değerini (çalışma modu):"// Google

#define S10500	"Monitör"
#define S10501	"&Veri portu(+0)"
#define S10502	"oku"
#define S10503	"&durum portu (+1)"
#define S10504	"&yönlendirme portu(+2)"
#define S10506	"Bayt &okuma"
#define S10507	"&Adres"
#define S10508	"Da&ten"
#define S10509	"Byte okumak"
#define S10512	"FIFO dolu"
#define S10513	"FIFO boş"
#define S10514	"yön kaydı (USB2LPT özgü)"

// STRINGTABLE 	// Emulation + İstatistikler
#define S16 "Gerçekten bu standart ölçülü port adresini kullanmak istiyor musunuz?\n\nBu değer ile bilgisayarı Nirvana'ya gönderecek değeri elinizde bulunduruyorsunuz!"
#define S17 "Verilen port adresi geçerli bir heksademik bir sayı değil!"
#define S18 "Port adresleri geçerli değil: çok küçük, çok büyük ya da 4'e bölünemiyor!"
#define S19 "8'e bölünemeyen port adreslerinde EPP-Emulasyonu mümkün değil!"
#define S20 "Zaman limitinin aşılmasında geçersiz özellikler! (1000 ms den büyük olamaz.)"
#define S21 "emulasyon port adreslerinin düzenlenmesi"
#define S22 "Port adresleri gereksinimlerde bildirilen kullanılan yazılım ile uyuşmalıdır."
#define S23 "Firmware (USB2LPT-Cihaz'ındaki yazılım) Feature-Register'ı desteklemez!"
#define S24 "none"	// Seriennummer
#define S25 "Bağlam &Yardım"
#define S26 "Bulk &transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\nİlerle?"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\nİlerle?"

// Firmware Güncelleme
#define S32 "Firmware başarılı bir şekilde EEPROM'a yazıldı. Yeni firmware'i çalıştırmak için cihazı takın ve sökün."
#define S33 "Firmware kaydında belirsiz hatalar. Çıkarmayın, tekrar deneyin. Cihaz artık çalışmayabilir!"
#define S34 "«%s» verisi artık bu USB2LPT-Cihası için firmware değil!"
#define S35 "Güncel Firmware'in tarihi: %s\n\nFirmwar'in tarihi yazılacak: %s\n\nİlerle?"
#define S36 "bilinmiyor"		// Firmware-tarihi (önceki çıkışlar tarih içermiyordu)
#define S37 "Firmware\0*.iic"		// Veri-açma-stardart diyoloğu filtresi
#define S38 "kusursuz fonksiyon."
#define S39 "Hatalı işlem görüldü! Kurzschlusstest.EXE detayları belirtir."
#define S40 "Her 17 bağlantıda yüksek potansiyel var mı?"
#define S41 "Düşük için bellenim-ATmega denetleyicisiyle burada güncellenen olamaz USB2LPT cihaz Hız, ancak etkinleştirmek için bir bayrak ayarlayabilirsiniz bir önyükleyici bulunmaktadır. Kullandığınız güncelleştirmeli güç bisiklet sonra, ""bootloadHID.EXE"".\n\nİlerle?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

// Monitör
#define S48 "SPP (Nibble)\0Çift yönlü\0Otomatik Strobe\0ECP\0EPP\0saklıdır\0Deneme\0Yapılandırma"
#define S49 "ek\0saklıdır\0görev çağrısı talebi\0hata\0çalıştırıldı\0sayfa sonu\0onaylayın\0meşgul"
#define S50 "gözleme\0otomatik besleme\0geri alma\0seçme\0görevden serbestlik\0Veri yönü: 1 = giriş\0uygulanmadı"
#define S51 "&Komuta\0&Adres"		// Bant seçicinin yazılması (Radyo butonu)
#define S52 "normal\0Nibble modu\0Baytlık modu\0ECP modu\0EPP modu"

// VERSIONINFO
#define I002	31,1
#define S003	"Özellik sayfasında sağlayıcınıza"	// FileDescription
#define S005	"Freeware"			// LegalCopyright
#define S006	"h#s USB LPT için çevirici"	// ProductName
#define S007	"çekirdek modu sürücüsü"		// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s USB LPT için çevirici için sürücü disk"
DevDesc ="h#s USB LPT için çevirici"
SvcDesc ="h#s USB LPT için çevirici için sürücüsü (usb2lpt.sys)"
*/
