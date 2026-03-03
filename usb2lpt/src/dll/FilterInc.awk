#AWK-Skript zum Filtern der RC-Include-Dateien (UTF-8)
#Übergabevariablen:
# VER=Dateiversion: hauptversion,nebenversion,jahr,monat*100+tag
# VP =Produktversion
# CP =codepage
#Aus der INC-Datei wird "#define I002" (Sprache/Subsprache) herausgefischt.
#Sowie "#define S004" (Versionsformatierungsstring)
#Wenn nicht vorhanden, dann ist S004 == "%d.%02d (%d/%02d);1;2;4/;3%",
#das ergibt "1.02 (5/09)" mit Monat (ohne Tag, daher /100) und Jahr (ohne Jahrhundert, deshalb %100),
#mit der Angabe der Reihenfolge der Argumente
#Ziel ist vor allem die Versionsnummernsteuerung zentral per makefile

#Heraus kommen einige weitere #define-Zeilen:
# I000: Sprache als Hexzahl, KOMMA, Codepage wie CP-Variable
# S000: StringFileInfo-Blockname
# I008: FileVersion, wie VER-Variable
# I009: ProductVersion, wie VP-Variable

$2=="I002"{
 LANG=$3		# Sprache/Subsprache (numerisch!) herausfischen
}

$2=="S004" {
 FORMAT=gensub(/^.*"([^"]*)".*$/,"\\1",0);
}

/^\/\*/,/\*\/$/{	# Mehrzeilen-Kommentar löschen (ist nur für .INF-Datei zum Anhängen)
 next;
}

{
 print;
}

function MakeVersionString(ver, n,i,v,a,op) {
 split(ver,v,",");
 n=split(FORMAT,f,";");
 for (i=2; i<=n; i++) {
  a[i]=v[substr(f[i],1,1)+0];	# Argumente in angegebener Reihenfolge
  op=substr(f[i],2,1);		# Operation mit Hundert
  if (op=="/") a[i]=a[i]/100;
  if (op=="%") a[i]=a[i]%100;
 }
 return sprintf(f[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9])
}

END{
 split(LANG,l,",");
 lng=l[2]*1024+l[1];
 print "#define S000 " sprintf("\"%04X%04X\"",lng,CP);
 print "#define I000 " lng "," CP;
 print "#define I008 " VER;
 print "#define I009 " VP;
 if (!FORMAT) FORMAT="%d.%02d (%d/%02d);1;2;4/;3%";
# print FORMAT
 print "#define S008 \"" MakeVersionString(VER) "\"";
 print "#define S009 \"" MakeVersionString(VP) "\"";
}
