#AWK-Skript zum Erzeugen mehrsprachiger RC-Dateien (UTF-8)
#Übergabevariablen:
# VER=Dateiversion: hauptversion,nebenversion,jahr,monat*100+tag
# VP =Produktversion
# CP =codepage
#Aus den INC-Dateien wird "#define I002" (Sprache/Subsprache) herausgefischt.
#Sowie "#define S004" (Versionsformatierungsstring)
#Wenn nicht vorhanden, dann ist S004 == "%d.%02d (%d/%02d);1;2;4/;3%",
#das ergibt "1.02 (5/09)" mit Monat (ohne Tag, daher /100) und Jahr (ohne Jahrhundert, deshalb %100),
#mit der Angabe der Reihenfolge der Argumente
#Ziel ist vor allem die Versionsnummernsteuerung zentral per makefile

#Besondere Aufmerksamkeit erfordert die VERSIONINFO-Ressource:
# I000: Sprache als Hexzahl, KOMMA, Codepage wie CP-Variable
# S000: StringFileInfo-Blockname
# I008: FileVersion, wie VER-Variable
# I009: ProductVersion, wie VP-Variable

BEGIN{
# Liste der verfügbaren Sprachen erzeugen, füllt <incfile>-Array
 IGNORECASE=1;
 kdo="ls -1 dll/*.inc";		# 1 Dateiname pro Zeile
 k=1;
 while ((kdo|getline)>0) {
  if ($0 ~ /en/) incfile[0]=$0;	# englisch zuerst
  else incfile[k++]=$0;
 }
 IGNORECASE=0;
# Alle .inc-Dateien einlesen und aus #define-Zeilen assoziatives Array <data> erzeugen
 for (i=0; i<k; i++) {
  FORMAT="%d.%02d (%d/%02d);1;2;4/;3%";	#Standard-Format: Version.Subversion (Monat_ohne_Tag/Jahr_ohne_Jahrhundert)
  data[i,"S10013"]="\"ms\""
  while ((getline<incfile[i])>0) {
   if ($1 == "#define" && $2 ~ /[SI][0-9]+/) {
    i2=$2;
    sub(/^#define[ \t]+\w+[ \t]+/,"");	# $1 und $2 löschen
    sub(/[ \t]*\/\/.*$/,"");	# C++-Kommentar löschen
    data[i,i2]=$0;		# String in Hochkommata (oder Zahl ohne) merken
    if (i2=="S004") FORMAT=gensub(/^.*"([^"]*)".*$/,"\\1","");	# Hochkommas entfernen
   }
  }
  split(data[i,"I002"],l,",");
  lng=l[2]*1024+l[1];
  data[i,"S000"]=sprintf("\"%04X%04X\"",lng,CP);
  data[i,"I000"]= lng "," CP;
  data[i,"I008"]= VER;
  data[i,"I009"]= VP;
  data[i,"S008"]= "\"" MakeVersionString(VER) "\"";
  data[i,"S009"]= "\"" MakeVersionString(VP) "\"";
 }
}

/^#ifdef WIN32/ {
 next;
}

/^#ifdef _DEBUG/,/^#else/ {
 next;
}

/^#else/,/^#endif/ {
 next;		# Win16 ausfiltern
}

/^#ifndef S10013/,/^#endif/ {
 next;		# ausfiltern (Default = "ms", siehe oben)
}

/^LANGUAGE I002/,/^LANGUAGE 0,0/ {
 line[ln++]=$0;	# aufsammeln
 next;
}

/^#endif/ {	#nach LANGUAGE 0,0 oder 2 24 "prop.manifest"
 if (ln) for (i=0; i<k; i++) SubstLines(i);
 ln=0;
 next;
}

/BLOCK S000/,/}/ {
 line[ln++]=$0;
 if ($0 ~ /}/) {
  for (i=0; i<k; i++) SubstLines(i);
  ln=0;
 }
 next;
}

/  VALUE "Translation", *I000/ {
 printf("  VALUE \"Translation\"");
 for (i=0; i<k; i++) printf(",%s",data[i,"I000"]);
 print ""
 next;
}

/ (FILE|PRODUCT)VERSION / {
 print SubstLine(0,$0);
 next;
}

{
 print	# unverändertes
}

function MakeVersionString(ver, n,i,v,a,op) {
 split(ver,v,",");		# Version = Version,Subversion,Jahr,Monat*100+Tag
 n=split(FORMAT,f,";");
 for (i=2; i<=n; i++) {
  a[i]=v[substr(f[i],1,1)+0];	# Argumente in angegebener Reihenfolge
  op=substr(f[i],2,1);		# Operation mit Hundert
  if (op=="/") a[i]=a[i]/100;
  if (op=="%") a[i]=a[i]%100;
 }
 return sprintf(f[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9])
}

function SubstLine(lang,line, k,t) {
 if (match(line,/[SI][0-9]+/)) {
  k=substr(line,RSTART,RLENGTH);
  t=data[lang,k];		# Ersetzungs-String sollte hier drin stehen
  if (t) line=substr(line,1,RSTART-1) t substr(line,RSTART+RLENGTH);
 }
 return line;
}

function SubstLines(lang, i) {
 for (i=0; i<ln; i++) print SubstLine(lang,line[i])
}
