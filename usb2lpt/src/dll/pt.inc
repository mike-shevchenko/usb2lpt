// Übersetzung von Ewerton Luiz Costadelle <ewertonluiz@gmail.com>

// DIALOG(EX)
#define S10000	"Emulação"
#define S10001	"Endereço da porta emulad&a:"
#define S10002	"Modo LPT m&elhorado:"
#define S10003	"Método de captura para instruções IO"
#define S10004	"&Depurar captura registro"
#define S10005	"(Pegar todos as instruções de IO)"
#define S10006	"&Forçar atribuitos, mesmo que anteriormente atribuídos"
#define S10007	"Redirecionar &READ/WRITE_PORT_UCHAR/USHORT/ULONG"
#define S10008	"(Mais rápido, mais estável)"
#define S10009	"Opções de desempenho"
#define S10010	"U&se gravação para instruções OUT"
#define S10011	"(Concatenar múltiplos instruções OUT para único pacote de USB)"
#define S10012	"for&zar a enviar los paquetes para"
#define S10014	"Retornar cópia local dos registradores de saída"
#define S10015	"Mais pr&ecisamente…"
#define S10016	"(Retornar o último valor escrito na instrução IN, não funciona quando as saídas são conduzidos [baixo] por hardware externo)"

#define S10100	"Estatística"
#define S10101	 "Conexión de los registros de debug"
#define S10102	"&Restaurar"
#define S10103	"Instruções de saída:"
#define S10104	"Instruções de entrada:"
#define S10105	"Erros de emulação:"
#define S10106	"(instrução IO não suportada, ex.: OUTSB)"
#define S10107	"Registros depurados roubados:"
#define S10108	"(Um driver desconhecido solicitou depuração de registros)"
#define S10109	"Registos de depuração sobrescritos são detectados por um timer de kernel de 100ms."
#define S10110	"de hecho asignado a:"
#define S10114	"redirecionamento READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10115	"Res&taurar"
#define S10116	"Maior que o padrão:"
#define S10117	"USB2LPT &especial…"
#define S10118	"Atualização de &firmware…"
#define S10119	"haftmann#software, Technische Universität Chemnitz,\nFakultät für Elektrotechnik und Informationstechnik,\nProfessur für Mess- und Sensortechnik, 2006-2010"

#define S10200	"Retornar cópia dos registros de saída"
#define S10201	"Porta de &dados (endereço base + 0)"
#define S10202	"Porta de &controle (endereço base + 2)"
#define S10203	"&Outras portas (ECP, EPP)"
#define S10204	"OK"
#define S10205	"Cancelar"

#define S10302	"Uma configuração errada pode travar o computador ou apagar o conteúdo do seu HD! Recomendamos usar um dos três endereços pré-definidos."
#define S10304	"Modos melhorados permitem a captura de endereços de portas adicionais. São muito pouco utilizados, então a configuração SPP está OK."
#define S10305	"Estas e outras configurações que você pode fazer mais tarde no Gerenciador de dispositivos do Windows. Aponte para o dispositivo USB2LPT e abra suas propriedades."

#define S10400	"USB2LPT especial"
#define S10401	"Registro de recurso"
#define S10402	"(O registo de recursos será mantidos na EEPROM do dispositivo USB2LPT. Acesso via DeviceIoControl)"
#define S10403	"Abrir coletores para porta de &dados (+0)"
#define S10404	"Abrir coletores para porta de &controle (+2)"
#define S10405	"Desligar &Pullups (e 5V no Pin25 em caso conectado)"
#define S10406	"Modo I&O direto (converte USB2LPT em dispositivo de IO)"
#define S10407	"(5V fraco, em vez de 3.3V forte para nível lógico alto)"
#define S10408	"(A marca cinza significa: Abrir coletores no modo SPP, Push-Pull invertido)"
#define S10409	"(Economia de energia)"
#define S10410	"(Não há mais controle invertido e linhas de status! Portanto, sem emulação útil, para o acesso via DeviceIoControl.Rev.4 tem 20 linhas de IO, 17 inversos. Direção é selecionáveis por bit e por pino.)"
#define S10411	"Defina && car&regue"
#define S10412	"Procurar c&urtos-circuitos"
#define S10413	"Número de série"
#define S10414	"no descritor US&B"
#define S10415	"Data do Firmware"
#define S10416	"&LED azul esmaecido"
#define S10417	"Valor de inicialização para &ECR (modo de operação):"

#define S10500	"Monitor"
#define S10501	"Porta de &dados (+0)"
#define S10502	"Ler"
#define S10503	"Porta de &status (+1)"
#define S10504	"Porta de &Controle (+2)"
#define S10506	"&Ler Byte"
#define S10507	"En&dereço"
#define S10508	"D&ados"
#define S10509	"&Escrever Byte"
#define S10512	"FIFO completa"
#define S10513	"FIFO vazio"
#define S10514	"Direção de registro (específico USB2LPT)"

// STRINGTABLE 	// Emulation+Statistik
#define S16 "Você REALMENTE DESEJA utilizar este endereço de porta não-padrão? Você pode travar seu computador!"
#define S17 "Este endereço de porta não é um hexadecimal válido!"
#define S18 "Este endereço de porta está errada: muito pequeno, muito grande ou não divisível por 4!"
#define S19 "Não é possível emular EPP em uma porta endereços não divisível por 8!"
#define S20 "Valor de tempo limite incorreto! (Não pode ser maior que 1000 ms.)"
#define S21 "Atribuição de endereço de porta emulada"
#define S22 "Este endereço de porta deve corresponder ao endereço usado pelo seu aplicativo."
#define S23 "Este firmware não suporta o recurso de registrar USB2LPT específico!"
#define S24 "nenhum"	// Seriennummer
#define S25 "Contexto de &ajuda"
#define S26 "Bul&k transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\nContinuar?"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\nContinuar?"

#define S32 "O firmware foi escrito na EEPROM com êxito.\nReinicie o dispositivo para iniciar o novo firmware."
#define S33 "Erro desconhecido ao gravar firmware para EEPROM. Por favor, não desligue dispositivo e tente novamente. O dispositivo USB2LPT não pode funcionar mais!"
#define S34 "O arquivo “%s” não foi aceito como firmware para esse dispositivo!"
#define S35 "Data do firmware em execução: %s\n\nData do novo firmware: %s\n\nContinuar?"
#define S36 "desconhecido"		// Firmware-Datum (frühe Ausgaben enthielten kein Datum)
#define S37 "Firmware\0*.iic"		// Filter für Datei-öffnen-Standarddialog
#define S38 "função Perfect."
#define S39 "Detectamos uma função incorreta! Execute Kurzschlusstest.exe para mais detalhes."
#define S40 "Você mediu todas as 17 linhas em nível alto?"
#define S41 "O firmware de baixa velocidade, com o controlador de dispositivo USB2LPT ATmega, não pode ser atualizado aqui. Mas você pode definir para ativar um bootloader embutido. Após reiniciar o dispositivo você deve atualizar usando “bootloadHID.EXE”.\n\nContinuar?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

#define S48 "SPP (Nibble)\0Bidirecional\0AutoStrobe\0ECP\0EPP\0reservado\0modo de teste\0Configuração"
#define S49 "adicional\0reservado\0Solicitação de interrupção\0Erro\0Online\0Livro Fim\0Reconhecer\0ocupado"
#define S50 "Strobe\0AutoFeed\0Init\0Selecionar Entrada\0Ativar Interrupção\0Direção de dados, 1=input\0não implementado"
#define S51 "C&omando\0&Endereço"	// Beschriftung für Wellenschalter (Radiobutton) 105/122
#define S52 "normalmente\0Modo Nibble\0Modo Byte\0Modo ECP\0Modo EPP"

// VERSIONINFO
#define I002	22,1
#define S003	 "Provedor de Página de propriedades"	// FileDescription
#define S005	"Domínio Público"			// LegalCopyright
#define S006	"h#s conversor USB para LPT"		// ProductName
#define S007	"Driver de modo de núcleo"		// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s Disco de controladores para convertidor de USB a LPT"
DevDesc ="h#s convertidor de USB a LPT"
SvcDesc ="h#s Driver para convertidor de USB a LPT (usb2lpt.sys)"
*/
