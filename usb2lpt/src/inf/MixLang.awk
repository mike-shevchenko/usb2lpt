#AWK-Skript zum Erzeugen der mehrsprachigen INF-Datei (UTF-8)

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
# Alle .inc-Dateien einlesen und den Blockkommentar ausspucken
 for (i=0; i<k; i++) {
  while ((getline<incfile[i])>0) {
   if ($2 == "I002" && i) {	# außer für Englisch
    print ""
    split($3,l,",");
    printf("[Strings.%04X]\n",l[2]*1024+l[1]);
    print "h#s     =haftmann#software"
   }
   else if ($0 ~ /\/\*/) o=1;
   else if ($0 ~ /\*\//) o=0;
   else if (o) print
  }
 }
}
