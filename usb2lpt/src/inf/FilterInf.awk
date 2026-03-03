#AWK-Skript zum Filtern der Ergebnis-INF-Datei
#Übergabevariablen:
# VER_INF=hauptversion,nebenversion,jahr,monat*100+tag
# HLP_LNG=hilfedateisprache, "" oder "en" oder "de" oder "pl" oder "en de pl"
#Heraus kommt eine reine ASCII-INF-Datei

BEGIN {
 FS="";		# kein Feld-Separator
 split(VER_INF,v,",");
 DriverVer=sprintf("%02d/%02d/%04d,%d.%02d.%04d.%04d",v[4]/100,v[4]%100,v[3],v[1],v[2],v[3],v[4]);
 helpsrc="usb2lpt.hlp";		# später vielleicht .chm
 if (HLP_LNG) k=split(HLP_LNG,langs," ");
}

/^;/{		# Ganzzeilen-Kommentar zeilenweise löschen
 next;
}

/\$HELPSRC\$/ {
 if (k) {
  for (i=1; i<=k; i++) {
   print gensub(/\$HELPSRC\$/,"..\\\\" langs[i] "\\\\" helpsrc,0);
  }
  next;
 }else{
  sub(/\$HELPSRC\$/,helpsrc);
 }
}

/\$COMMA_HELPSRC\$/ {
 if (HLP_LNG) {
  if (k>1) {
   for (i=1; i<=k; i++) {
    print gensub(/\.hlp\$COMMA_HELPSRC\$/,"." langs[i] ".hlp,..\\\\" langs[i] "\\\\" helpsrc,0);
   }
   next;
  }else sub(/\$COMMA_HELPSRC\$/,",..\\" HLP_LNG "\\" helpsrc);
 }else sub(/\$COMMA_HELPSRC\$/,"");
}

/usb2lpt\.(sys|dll)=1/ {
 if (k>1) next;
}

/\$Chicago\$/ {
 if (k>1) sub(/\$Chicago\$/,"Windows NT");
}

{
 sub(/\$VER_INF\$/,DriverVer);
 if (k>1 && $0 ~ /@$/) next;	# Win9x-Einträge löschen
 sub(/\t*;.*/,"");	# Angehängten Kommentar löschen
 print;
}
