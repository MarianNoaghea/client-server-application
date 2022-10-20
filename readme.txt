  In aceasta tema am reusit sa aprofundez ideea de protocol de nivel aplicatie si am dobandit mai
multe cunostinte despre lucrul cu socketi, multiplexare si modul in care sunt transportate datele.
Protocolul se foloseste de structuri de date special create pentru receptionarea si "servirea"
 mai departe a acestora catre clientii TCP, clientii folosindu-se mai departe de aceste strucri
pentru intelegerea mesajului.
  Aplicatia de tip client-server se foloseste de socketi.
Serverul:
  Este un process unic care realizeaza legatura intre clientii UDP si TCP, acesta se ocupa si de
tinerea in evidenta a subscriptilor si a mesajelor in asteptare.
Sochetii folositi sunt unul pentru citirea de la tastatura si doi pentru comunicarea cu UDP si
TCP luati in evidenta cu ajutorul unui for pana la fdmax.
  Pentru scrierea serverului am plecat de la scheletul de cod din laboratorul 8, de la care am
adaugat functionalitatea de receptie a mesajelor de tip UDP cu formatul definit in enuntul
temei. Pentru aceasta am creat o structura numita "msg" conforma cu structura mesajului udp, 
astfel am obtinu mult mai usor octetii din recvfrom.
  Odata receptionate, datele sunt trimise mai departe catre subscriberi cu ajutorul functiei
"send_to_subscribers". Pentru evidenta subscriberilor am creeat o structura noua numita "client"
in care retin informatiile despre clientii conectati, topicurile la care sunt abonati si cu ce
obtiune(SF sau nu), dar si un vector de mesaje ce sunt in asteptare.
  Toti subscriberii sunt retinuti intr-un vector de clienti, iar toate topicurile sunt retinute
intr-un vecotr de topicuri.
send_to_subscribers trimite mesajele mai departe cu ajutorul altei structuri "msg_srv" aceasta
contine un mesaj "msg" dar si informatii despre ip, port si cel mai important size-ul(cu acesta
pot trimite exact cata informatie este nevoie ci nu mai mult si subscriber-ul la fel va stii cata
informatie sa receptioneze).
  Mesajele ce nu pot fi transmise mai departe vor fi puse intr-un vector de "msg_srv" (daca
clientul respectiv are SF = 1). Verificarea SF-ului se face in O(1) cu ajutorul unui Map din
cadrul clientului care va avea key = numele topicului, si valoarea = SF.
Structura "topic" contine numele acestuia dar si un vector de pointeri care pointeaza spre
clientii  care sunt abonati la acesta, astfel trimiterea este una mult mai rapida deoarece la
sosirea unui mesaj send_to_subscribers se uita doar la clientii ce sunt abonati la topicul
respectiv ci nu la toti clientii si la toate topicurile.
  La prima conectare a unui client TCP acesta trimite ID_NAME-ul, acesta este adaugat in vectorul
de clienti(daca este un client nou). Clientii TCP mai pot trimite mesaje de subscribe
/unsubscribe, mesajele de felul sunt procesate cu ajutorul functiei "process_request" care
gestioneaza astfel:
daca este un mesaj de subscribe va verifica daca topicul exista deja, 
daca acesta nu exista se creeaza unul si acesta va avea un pointer spre acel client,
daca topicul exista se verifica daca clientul este deja abonat si daca da in situatia in care
cererea de abonare are un SF diferit de cel prezent acesta se va reabona la topic cu SF-ul
updatat. Daca este un mesaj de unsubscribe se va cauta in lista de clienti a topicului respectiv
si se va sterge pointerul spre client.
O functie foarte utila a fost "generate_message" cu care creez o structura de tip "msg_srv" dintr-
un string pentru a trimite mesaje de informare in legatura cu cererea facuta de clientul TCP.
Daca serveul primeste pe socketul de tastatura comanda exit acesta trimite mesaj catre toti
clientii conenctati "Server is closed." mesaj la care clientii isi vor inchide procesul.

Clientul:
Acesta va receptiona intr-un buffer pe socketul de tastatura si intr-o structura msg_srv pe
socketul serverului. La primirea unui mesaj din partea UDP mai intai se va primi sizeul, dupa
care se va primii si restul mesajului cu dimensiunea aflata.
Cu ajutorul functiei process payload si a unor functii de conversie a datelor din siruri de
octeti in valoarea efectiva afisez mesajul primit.
Functiile de conversie se folosesc de cast la uint_t si de conversia din network order in host
order.

Problemele intampinate au fost la gestionarea si accesarea memoriei din program.
Durata de realizare a temei s-a intins pe aproape 5 zile.
