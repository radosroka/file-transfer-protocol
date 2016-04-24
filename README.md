README
===============

Program, ktorý umožňuje za využitia socketov jednoduché odosielanie a 
sťahovanie súborov medzi klientom a serverom.

SERVER
===============

Pre spustenie serveru je potrebné zadať čislo portu, na ktorom potom
server beží a čaká na prichadzajúce pripojenia. Pri pripojení klienta
na server je klientovi vytvorené nové vlákno, ktoré slúži pre jeho
obsluhu. Po vytvorení vlákna následuje komunikácia popísaná v súbore
ipk.pdf.

Počas komunikácie môže dojsť k neúspešnemu ukončeniu serveru v prípade
chyby pri komunikácii medzi klientom a serverom. 

Pre ukladanie dát server využíva dočastný súbor. Po úspešnom prijatí
dát sa tento súbor premenuje na správny názov. V prípade neúspechu
sa dočastný súbor odstraní. To zabezbečuje riešenie situácie pri
ktorej je server aj klient v rovnakej zložke.

Po úspečnom prenose sa vlákno ukončí.

Príklad spustenia serveru na porte 10001:
./server -p 10001

KLIENT
===============

Pre spustenie klienta je potrebné zadať názov serveru, na ktorý sa 
má pripojiť, číslo portu a taktiež typ požiadavku (upload alebo 
download) spolu s názvom súboru.

Po spustení následuje komunikácia popísana v súbore ipk.pdf.

Príklad stiahnutia súboru test.txt zo serveru bežiacom na eva.fit.vutbr.cz
na porte 10001
./client -h eva.fit.vutbr.cz -p 10001 -d test.txt

Príklad odoslania súboru test.txt na server bežiaci na eva.fit.vutbr.cz
na porte 10001
./client -h eva.fit.vutbr.cz -p 10001 -u test.txt
