// translated by Henrik Haftmann
// Revised by Masahiko Niwa, 130916

// DIALOG(EX)
#define S10000	"エミュレーション"
#define S10001	"仮想ポート･アドレス (&A)："
#define S10002	"LPT拡張モード (&E)："
#define S10003	"IN・OUT命令のトラップ方法"
#define S10004	"デバッグ･レジスタートラップ (&D)"
#define S10005	"(すべてのIN・OUT命令をトラップします)"
#define S10006	"前回の割り当があっても、割り当てを強制する (&F)"
#define S10007	"READ/WRITE_PORT_UCHAR/USHORT/ULONG リダイレクト(&R)"
#define S10008	"(より速く、より安定的)"
#define S10009	"パフォーマンス･オプション"
#define S10010	"OUT命令についてライトバックを使用する (&W)"
#define S10011	"(複数のOUT命令を1つのUSBパケットに連結します)"
#define S10012	"パケット送信までの、タイムアウト時間 (&T)："
#define S10014	"ローカルで保持しているOUTレジスターの値を読み込む (&B)"
#define S10015	"詳細オプション (&M)"
#define S10016	"(最後に出力したOUT命令のデータを、IN命令使用時にそのまま読み込みます。パラレルポートが外部デバイスから駆動されている場合は機能しません。)"

#define S10100	"統計"
#define S10101	"デバッグ･レジスタートラップ"
#define S10102	"ゼロに初期化 (&R)"
#define S10103	"OUT命令："
#define S10104	"IN命令："
#define S10105	"エミュレーション･エラー："
#define S10106	"(非対応のIN、またはOUT命令。例えば、OUTSB)"
#define S10107	"盗まれたデバッグ･レジスター："
#define S10108	"(未知のドライバがデバッグ･レジスターに書込みをおこなった)"
#define S10109	"デバッグ･レジスターの上書きは、100ms周期のカーネルタイマーで監視されています。"
#define S10110	"実際の割り当て："
#define S10114	"READ_PORT_UCHAR/WRITE_PORT_UCHAR リダイレクト"
#define S10115	"ゼロに初期化 (&Z)"
#define S10116	"サイズ超過："
#define S10117	"USB2LPT固有の設定... (&E)"
#define S10118	"ファームウェア･アップデート... (&U)"
#define S10119	"haftmann#software、ケムニッツ工科大学、電子情報工学部　センサー技術講座、2006-2009年"

#define S10200	"出力レジスターのリードバックコピー"
#define S10201	"データ･ポート(base address + 0) (&D)"
#define S10202	"コントロールポート(base address + 2) (&C)"
#define S10203	"その他のポート (ECP,EPP等) (&O)"
#define S10204	"OK"
#define S10205	"Cancel"

#define S10302	"間違った設定を行うと、コンピュータがクラッシュしたり、ハードディスクの内容が消去される可能性があります! 三つの定義済アドレス使用は安全です。"
#define S10304	"LPT拡張モードを使用すると、追加のポートアドレスが有効になります。\nこの設定はめった使用されないため、ほとんどの場合、SPPモードでの使用をお勧めします。"
#define S10305	"Windowsのデバイス･マネージャから、USB2LPTデバイスのプロパティを開くことにより、より詳細な設定を行うことが可能です。"

#define S10400	"USB2LPT固有の設定"
#define S10401	"Feature Register"
#define S10402	"(Feature Registerの設定はUSB2LPTのEEPROMに永続的に保存されます。これはDeviceIoControlで設定します。)"
#define S10403	"データ･ポート(+0)をオープンコレクタに設定します (&D)"
#define S10404	"コントロール･ポート(+2)をオープンコレクタに設定します(&C)"
#define S10405	"プル･アップをオフにする(結線されている場合、25番ピンは5V) (&P)"
#define S10406	"Direct I/Oモード（USB2LPTをI/Oデバイスとして扱う(&I)"
#define S10407	"(HIGHレベルの出力に、3.3Vの代わりに、5Vを使用しますが、出力が低下します)"
#define S10408	"(グレーのチェックマークの意味：SPPモード時はオープンコレクタ、それ以外はプッシュ･プル)"
#define S10409	"(省電力)"
#define S10410	"(信号を反転しない。LPTのエミュレーションとしては正しくないため、DeviceIoControlによるアクセス用。新しいUSB2LPTでは、従来の17本に替わり、20本のI/Oを持っている。方向選択はピンごとに可能。)"
#define S10411	"設定と読込 (&S)"
#define S10412	"短絡を検出する (&T)"
#define S10413	"シリアル番号"
#define S10414	"USBディスクリプタの中(&U)"
#define S10415	"ファームウェアの日付"
#define S10416	"青色LEDの減光(&L)"
#define S10417	"ECRの初期値（動作モード）:(&E)"

#define S10500	"モニター"
#define S10501	"データ･ポート(+0) (&P)"
#define S10502	"読み込み"
#define S10503	"ステータス･ポート(+1) (&S)"
#define S10504	"コントロール･ポート(+2) (&C)"
#define S10506	"Read Byte (&R)"
#define S10507	"Address (&A)"
#define S10508	"Data (&D)"
#define S10509	"Write Byte (&W)"
#define S10512	"FIFO full"
#define S10513	"FIFO empty"
#define S10514	"Direction register（USB2LPT固有)"

// STRINGTABLE 
#define S16 "本当に、この非標準のポートアドレスを使用しますか？\n\nお使いのコンピュータがクラッシュする可能性があります！"
#define S17 "ポートアドレスが有効な16進数ではありません！"
#define S18 "ポートアドレスが間違っています：小さすぎる、大きすぎる、または4で割り切れません！"
#define S19 "アドレスは8の倍数ではありません。EPPエミュレーションは使用できません。"
#define S20 "タイムアウト値が不正です！（1000ms以下の必要があります）"
#define S21 "仮想ポート･アドレスの割り当て"
#define S22 "このポート･アドレスはアプリケーションで使用するアドレスと一致する必要があります。"
#define S23 "このファームウェアはUSB2LPT固有機能のレジスター(feature register)をサポートしていません。"
#define S24 "none"	// serial number
#define S25 "コンテキスト･ヘルプ (&H)"
#define S26 "バルク転送 (より高速に動作します; XP以前のOSに適しています) (&k)"
#define S27 "インタラプト転送 ほか"
#define S28 "USB2LPTのバルク転送は、Windows Vista以降のOSで不具合があります!\nシステムのアップグレード前には、忘れずにこのビットをリセットしてください。\n\n続行しますか？"
#define S29 "デバッグ･レジスタトラップは、いくつかの分後PatchGuardによって検出される。何の対策が取られなかったとき、これはブルースクリーン0x109になる。\n\n続行しますか？"

#define S32 "ファームウェアはEEPROMに正常に書き込まれました。\n新しいファームウェアを起動するのために、デバイスの電源を落としてください。"
#define S33 "ファームウェアをEEPROMに書き込み中、不明なエラーが発生しました。デバイスの接続を解除せずに、もう一度実行してください。USB2LPTデバイスは動作していない可能性があります。"
#define S34 "ファイル「%s」はUSB2LPTデバイスのファームウェアではありません！"
#define S35 "実行中のファームウェアの日付：%s\n\n書き込まれるファームウェアの日付：%s\n\n続行しますか？"
#define S36 "不明"		// firmware date (early releases have no date inside)
#define S37 "ファームウェア\0*.iic"	// filter for open file standard dialog
#define S38 "完璧に機能します。"
#define S39 "不正な機能を検出しました！ 詳細は、Kurzschlusstest.EXEを起動してください。"
#define S40 "17本すべてのラインの HIGHレベルを測定しますか？"
#define S41 "本機能で、ATmegaコントローラの、低速版 USB2LPT用ファームウェアを更新することはできませんが、内臓ブートローダの設定フラグを有効にします。デバイスの電源再投入後、bootloadHID.EXEを使用してファームウェアを更新する必要があります。\n\n続行しますか？"
#define S42 "このUSB2LPTはUSB経由でのEEPROM書き換えをサポートしません。\n\n(First EEPROM byte = 0x%02X)"

#define S48 "SPP（ニブル）\0双方向\0自動ストローブ\0ECP\0EPP\0予約\0テストモード\0設定"
#define S49 "additional\0reserved\0Interrupt Request\0Error\0Online\0Paper End\0Acknowledge\0Busy"
#define S50 "Strobe\0AutoFeed\0Init\0Select Input\0Interrupt Enable\0Data direction, 1=input\0not implemented"
#define S51 "コマンド (&O)\0アドレス (&A)"	// Changing text for radio button 105/122
#define S52 "標準\0ニブルモード\0バイトモード\0ECPモード\0EPPモード"

// VERSIONINFO
#define I002	17,1
#define S003	"プロパティ･シート･プロバイダ"	// FileDescription
#define S004	"%d.%02d (%d年%d月);1;2;3;4/"		// FileVersion (awk-processed)
#define S005	"Public Domain"		// LegalCopyright
#define S006	"h#s USB to LPT converter"	// ProductName
#define S007	"カーネルモードドライバ"	// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s･USBからLPTまでのコンバータのドライバディスク"
DevDesc ="h#s･USBからLPTまでのコンバータ"
SvcDesc ="h#s･USBからLPTまでのコンバータのドライバ(usb2lpt.sys)"
*/
