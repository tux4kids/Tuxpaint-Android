                                   Tux Paint
                                     0.9.14

  Un semplice programma di disegno per bambini

                        Copyright 2004 by Bill Kendrick
                               New Breed Software

                           bill@newbreedsoftware.com
                   http://www.newbreedsoftware.com/tuxpaint/

                        14 giugno 2002 - 14 aprile 2004

               Traduzione a cura di Flavio "Iron Bishop" Pastore
                                 23 Aprile 2004

     ----------------------------------------------------------------------

                                    Indice:

     * Introduzione
     * Licenza d'uso
     * Descrizione
     * Documentazione Aggiuntiva
     * Usare Tux Paint
          * Compilare Tux Paint
          * Aprire Tux Paint
          * Opzioni
               * Opzioni da linea di comando
               * Opzioni descrittive da linea di comando
               * Scegliere una lingua differente
               * Specificare la lingua di sistema
               * Caratteri speciali
          * Schermata Iniziale
          * Schermata principale
          * Strumenti
               * Strumenti di disegno
                    * Pennello
                    * Timbro
                    * Linea
                    * Forma
                    * Testo
                    * Magia
                    * Gomma
               * Comandi
                    * Annulla
                    * Ripeti
                    * Nuovo
                    * Apri
                    * Salva
                    * Stampa
                    * Esci
     * Caricare altre immagini in Tux Paint
          * Usare 'tuxpaint-import'
          * Procedura manuale
     * Estendere Tux Paint
          * Dove vanno i file
          * Pennelli
          * Timbri
               * Immagine
               * Testo descrittivo
               * Effetto sonoro
               * Opzioni
               * Immagini pre-specchiate
          * Caratteri
     * Altre informazioni

     ----------------------------------------------------------------------

                                 Introduzione:

     "Tux Paint" e un programma di disegno per bambini. Mette a disposizione
     un'interfaccia semplice e una tela di dimensioni fisse; permette di
     accedere alle immagini precedentemente salvate attraverso piccole
     anteprime (nascondendo la struttura del file-system).

     A differenza di programmi di disegno famosi come "The GIMP", ha un
     insieme di strumenti assai limitato. D'altro canto mette a disposizione
     un'interfaccia semplice, dove trovano spazio particolari come gli
     effetti sonori, espressamente dedicati ai bambini.

     ----------------------------------------------------------------------

                                 Licenza d'uso:

     Tux Paint e un progetto Open Source, Software Libero rilasciato sotto
     GNU General Public License (GPL). Il codice sorgente del programma e
     disponibile (questo permette a chiunque di aggiungere funzioni,
     correggere errori e usare parti di codice nei propri programmi sotto
     GPL).

     Si veda il file docs/COPYING.txt per il testo completo della GNU GPL e
     il file docs/it/COPYING.it.txt per la traduzione non ufficiale in
     italiano.

     ----------------------------------------------------------------------

                                  Descrizione:

     Facile e divertente
             Tux Paint e pensato per essere un semplice programma di disegno
             per bambini, non uno strumento di disegno professionale. e
             pensato per essere diventente e facile da usare. Gli effetti
             sonori e un "aiutante disegnato" permettono all'utente di sapere
             cosa succede in ogni istante, limitando noia e ripetitivit`a. Ci
             sono anche puntatori del mouse dalla forma grande e fumettosa.

     Estensibilit`a
             Tux Paint e estensibile. Pennelli e timbri possono essere
             inseriti ed tolti dall'interfaccia. Ad esempio, un insegnante
             puo inserire nel programma un insieme di forme di animali e
             chiedere agli studenti di disegnare un ecosistema. Ad ogni forma
             puo essere assegnato un suono e un testo.

     Portabilit`a
             Tux Paint puo essere usato su molti sistemi operativi: Windows,
             GNU Linux, ecc. L'interfaccia rimane la stessa in ogni
             piattaforma. Tux Paint funziona bene anche su sistemi datati
             (come Pentium 133) e puo essere compilato appositamente per
             sistemi lenti.

     Semplicit`a
             Non c'e accesso diretto alla struttura dei dati sul computer.
             L'immagine corrente viene salvata quando si esce e riappare
             quando si riapre il programma. Per salvare una immagine non c'e
             bisogno di specificare un nome o usare la tastiera. Per
             recuperare una immagine basta selezionare la sua anteprima
             dall'elenco. Non si puo accedere ad altri file oltre a quelli
             specificati dal programma.

     ----------------------------------------------------------------------

                           Documentazione Aggiuntiva

     Documentazione inclusa in Tux Paint (nella cartella "docs/it/"):
       * AUTHORS.it.txt
         Lista degli autori e dei partecipanti allo sviluppo
       * CHANGES.it.txt
         Riassunto delle modifiche tra una versione e l'altra
       * COPYING.it.txt
         Licenza GNU General Public License (traduzione)
       * INSTALL.it.txt
         Istruzioni sull'installazione e sulla compilazione, dove applicabili
       * PNG.it.txt
         Note sulla creazione di immagini in formato PNG per l'uso in
         Tux Paint
       * README.it.txt
         (questo file)
       * TODO.it.txt
         Una lista di funzioni ancora da inserire o errori ancora da
         correggere

     ----------------------------------------------------------------------

                                Usare Tux Paint

Compilare Tux Paint

       Per compilare Tux Paint dal codice sorgente, fare riferimento al file
       INSTALL.it.txt.

     ----------------------------------------------------------------------

Aprire Tux Paint

  Utenti GNU Linux/Unix

         Usare il seguente comando al prompt della shell (es: "$"):

           $ tuxpaint

         e anche possibile creare un'icona di lancio (in GNOME o KDE su GNU
         Linux), fate riferimento alla documentazione del vostro Desktop...

         In caso di errori, verranno visualizzati nel terminale (su
         "stderr").

     ----------------------------------------------------------------------

  Utenti Windows

                                                          [Icon]              
                                                         Tux Paint            

         Fare doppio-click sull'icona "Tux Paint" nel desktop (che e stata
         creata dal programma di installazione) o fare doppio click sul file
         "tuxpaint.exe" nella cartella Tux Paint.

         In caso di errori, verranno salvati in un file chiamato "stderr.txt"
         nella cartella Tux Paint.

         Fare riferimento al file "INSTALL.it.txt" per dettagli su come
         personalizzare l'icona di collegamento di Tux Paint, per selezionare
         le opzioni el programma (da linea di comando).

         Per far partire Tux Paint e avere a disposizione direttamente le
         opzioni di linea di comando, avrete bisogno di usare il comando
         "tuxpaint.exe" da un prompt MSDOS o dalla finestra "Start" ->
         "Esegui" (fare riferimento al file "INSTALL.it.txt" per dettagli).

     ----------------------------------------------------------------------

  Utenti Mac OS X

         Semplicemente, fare doppio click sull'icona di "Tux Paint".

     ----------------------------------------------------------------------

Opzioni

  File di configurazione

           Potete creare un semplice file di configurazione per Tux Paint,
           che sar`a usato dal programma ad ogni avvio.

           Il file e un semplice file di testo contenente le opzioni che
           volete attivare:

    Utenti GNU Linux

             Il file da creare e chiamato ".tuxpaintrc" e deve essere messo
             nella cartella home (anche detta "~/.tuxpaintrc" oppure
             "$HOME/.tuxpaintrc")

             Quando questo file non viene creato, il programma legge un altro
             file (inizialmente, questa configurazione non ha opzioni attive)
             situato in:

               /etc/tuxpaint/tuxpaint.conf

             Potete disabilitare del tutto la lettura di questo file,
             lasciando le opzioni cos`i come sono inizialmente (che possono
             essere in seguito sovrascritte dal vostro file ".tuxpaintrc" e/o
             da argomenti di linea di comando) usando questa opzione nella
             riga di comando:

               --nosysconfig

    Utenti Windows

             Il file da creare e chiamato "tuxpaint.cfg" e deve essere messo
             nella cartella di Tux Paint.

             Potete usare NotePad o WordPad per creare questo file.
             Assicuratevi di salvarlo in formato solo testo, assicuratevi che
             il nome del file non abbia ".txt" al fondo...

    Opzioni disponibili

             Nel file di configurazione possono essere specificate le
             seguenti opzioni (gli argomenti dati da linea di comando
             sovrascriveranno questi, come spiegato nella sezione Opzioni da
             linea di comando).

             fullscreen=yes
                     Mostra il programma a tutto schermo, invece che in una
                     finestra.

             800x600=yes
                     Mostra il programma nella risoluzione 800x600
                     (SPERIMENTALE), invece che nella risoluzione solita
                     640x480.

             nosound=yes
                     Disabilita gli effetti sonori.

             noquit=yes
                     Disabilita il pulsante "Esci" sullo schermo (premere il
                     pulsante [Escape] o chiudere la finestra funziona
                     ancora).

             noprint=yes
                     Disabilita la funzione di stampa.

             printdelay=SECONDI
                     Limita la stampa in modo tale che possa essere usata
                     solo una volta ogni SECONDI secondi.

             printcommand=COMANDO

                     (Solo per GNU Linux e Unix)

                     Usa il comando COMANDO per stampare un file PNG. Se non
                     definito, il comando e:

                       pngtopnm | pnmtops | lpr

                     Converte PNG a NetPBM 'portable anymap', poi lo converte
                     in PostScript, infine lo manda alla stampante, usando il
                     comando "lpr".

             printcfg=yes

                     (Solo per Windows)

                     Tux Paint al momento di stampare user`a un file di
                     configurazione predefinito. Premendo il tasto [ALT]
                     mentre si cicka sul pulsante "Stampa" si puo far
                     apparire la finestra di configurazione della stampa di
                     Windows.

                     (Nota: questo funziona solo quando il programma non
                     viene aperto in modalit`a a tutto schermo. Tutte le
                     modifiche di configurazione fatte in questa finestra
                     verranno salvate nel file "userdata/print.cfg" e usate
                     ad ogni stampa successiva, finche l'opzione "printcfg"
                     rimane attiva.

             simpleshapes=yes
                     Disabilita la rotazione nello strumento "Forma".
                     Clickare, trascinare e rilasciare sar`a sufficiente per
                     disegnare una forma.

             uppercase=yes
                     Tutti caratteri verranno visualizzati maiuscoli (ad
                     esempio, "Pippo" diventer`a "PIPPO"). Utile per bambini
                     che sanno leggere, ma che per il momento hanno imparato
                     solo le maiuscole.

             grab=yes

                     Tux Paint prover`a a 'limitare' mouse e tastiera, in
                     modo che il mouse possa muoversi solo all'interno della
                     finestra di Tux Paint e gli imput di tastiera vengano
                     passati direttamente al programma.

                     Utile per disabilitare azioni verso il sistema operativo
                     che potrebbero far uscire l'utente dal programma, come i
                     comandi [Alt]-[Tab], [Ctrl]-[Escape], ecc. Utile
                     specialmente nella modalit`a a tutto schermo.

             noshortcuts=yes

                     Disabilita le scorciatoie da tastiera (ad esempio:
                     [Ctrl]-[S] per Salva, [Ctrl]-[N] per Nuovo, ecc).

                     E utile per prevenire l'uso di comandi indesiderati da
                     parte dei bambini che non hanno dimestichezza con la
                     tastiera.

             nowheelmouse=yes
                     Disabilita il supporto della "rotellina" per i mouse che
                     la possiedono (normalmente, la rotellina fa scorrere la
                     selezione nel menu di destra).

             nofancycursors=yes

                     Disabilita il puntatore del mouse di Tux Paint e
                     utilizza quello predefinito dal sistema operativo.

                     In alcuni sistemi puo succedere che il puntatore dia
                     problemi. In tal caso, usare questa opzione.

             nooutlines=yes

                     Questa modalit`a offre contorni piu semplici per gli
                     strumenti Linea, Forma, Stampiglia e Cancella.

                     Questo puo aiutare l'esecuzione di Tux Paint su computer
                     obsoleti, oppure quando viene mostrato in uno schermo
                     X-Window remoto.

             nostamps=yes

                     Fa in modo che il programma non carichi le immagini
                     degli stampi, disabilitando di conseguenza lo strumento
                     Stampiglia.

                     Questo velocizza l'apertura di Tux Paint e riduce l'uso
                     di memoria durante l'esecuzione.

             nostampcontrols=yes
                     Alcune immagini usate con lo strumento Timbro possono
                     essere specchiate, capovolte, o essere ingrandite e
                     rimpicciolite. Questa opzione disabilita queste
                     possibilit`a.

             mirrorstamps=yes

                     Stabilisce che tutti i timbri che possono essere
                     specchiati vengano specchiati automaticamente.

                     Puo essere utile a chi preferisce un orientamento
                     destra-verso-sinistra piuttosto che
                     sinistra-verso-destra.

             keyboard=yes

                     Permette di usare la tastiera per controllare il mouse
                     (utile ad esempio per sistemi senza mouse).

                     I tasti [Freccia] muovono il puntatore. Il tasto
                     [Spazio] funge da pulsante del mouse.

             savedir CARTELLA

                     Specifica quale cartella usare per salvare le immagini
                     create con Tux Paint. Le cartelle predefinite sono
                     "~/.tuxpaint/saved/" su GNU Linux e Unix, "userdata\" su
                     Windows.

                     Questo puo essere utile in un laboratorio con Tux Paint
                     installato su un server e i bambini lo usano da
                     workstation. Potete configurare savedir per essere una
                     cartella nella loro cartella principale (ad esempio,
                     "H:\tuxpaint\")

                     Nota: quando si specifica una unit`a di Windows (ad
                     esempio,"H:\"), bisogna anche specificare una
                     sottocartella.

                     Esempio: "savedir=Z:\tuxpaint\"

             saveover=yes
                     Disabilita la domanda "Sovrascrivere la vecchia
                     versione...?" quando si salva un file gi`a esistente.
                     Con questa opzione attivata, la vecchia versione viene
                     sempre rimpiazzata, automaticamente.

             saveover=new
                     Anche questa opzione disabilita la domanda
                     "Sovrascrivere la vecchia versione...?". In questo csao
                     pero, viene automaticamente creato un nuovo file e la
                     vecchia versione rimane salvata.

             saveover=ask

                     (Opzione superflua, in quanto corrisponde al
                     comportamento predefinito.)

                     Salvando un disegno gi`a esistente, verr`a chiesto se
                     sovrascrivere la vecchia versione o creare un nuovo
                     file.

             lang=LINGUA

                     Permette di usare Tux Paint in uno dei linguaggi
                     supportati. Le possibili scelte di LINGUA sono:

                       +----------------------------------------------------+
                       |english                |american-english  |         |
                       |-----------------------+------------------+---------|
                       |afrikaans              |                  |         |
                       |-----------------------+------------------+---------|
                       |basque                 |euskara           |         |
                       |-----------------------+------------------+---------|
                       |bokmal                 |                  |         |
                       |-----------------------+------------------+---------|
                       |brazilian-portuguese   |portuges-brazilian|brazilian|
                       |-----------------------+------------------+---------|
                       |breton                 |brezhoneg         |         |
                       |-----------------------+------------------+---------|
                       |british-english        |british           |         |
                       |-----------------------+------------------+---------|
                       |catalan                |catala            |         |
                       |-----------------------+------------------+---------|
                       |chinese                |                  |         |
                       |-----------------------+------------------+---------|
                       |czech                  |cesky             |         |
                       |-----------------------+------------------+---------|
                       |danish                 |dansk             |         |
                       |-----------------------+------------------+---------|
                       |dutch                  |nederlands        |         |
                       |-----------------------+------------------+---------|
                       |finnish                |suomi             |         |
                       |-----------------------+------------------+---------|
                       |french                 |francais          |         |
                       |-----------------------+------------------+---------|
                       |german                 |deutsch           |         |
                       |-----------------------+------------------+---------|
                       |greek                  |                  |         |
                       |-----------------------+------------------+---------|
                       |hebrew                 |                  |         |
                       |-----------------------+------------------+---------|
                       |hungarian              |magyar            |         |
                       |-----------------------+------------------+---------|
                       |icelandic              |islenska          |         |
                       |-----------------------+------------------+---------|
                       |indonesian             |bahasa-indonesia  |         |
                       |-----------------------+------------------+---------|
                       |italian                |italiano          |         |
                       |-----------------------+------------------+---------|
                       |japanese               |                  |         |
                       |-----------------------+------------------+---------|
                       |korean                 |                  |         |
                       |-----------------------+------------------+---------|
                       |lithuanian             |lietuviu          |         |
                       |-----------------------+------------------+---------|
                       |malay                  |                  |         |
                       |-----------------------+------------------+---------|
                       |norwegian              |nynorsk           |         |
                       |-----------------------+------------------+---------|
                       |polish                 |polski            |         |
                       |-----------------------+------------------+---------|
                       |portuguese             |portugues         |         |
                       |-----------------------+------------------+---------|
                       |romanian               |                  |         |
                       |-----------------------+------------------+---------|
                       |russian                |                  |         |
                       |-----------------------+------------------+---------|
                       |serbian                |                  |         |
                       |-----------------------+------------------+---------|
                       |spanish                |espanol           |         |
                       |-----------------------+------------------+---------|
                       |slovak                 |                  |         |
                       |-----------------------+------------------+---------|
                       |slovenian              |slovensko         |         |
                       |-----------------------+------------------+---------|
                       |swedish                |svenska           |         |
                       |-----------------------+------------------+---------|
                       |tamil                  |                  |         |
                       |-----------------------+------------------+---------|
                       |vietnamese             |                  |         |
                       |-----------------------+------------------+---------|
                       |turkish                |                  |         |
                       |-----------------------+------------------+---------|
                       |walloon                |walon             |         |
                       +----------------------------------------------------+

     ----------------------------------------------------------------------

    Sovrascrivere la configurazione predefinita. Opzioni mediante l'uso di
    .tuxpaintrc

             Se qualcuna delle opzioni descritte sono definite nel file
             "/etc/tuxpaint/tuxpaint.config", e possibile sovrascriverle nel
             proprio file "~/.tuxpaintrc".

             Per le opzioni vero/falso come "noprint" e "grab", si puo
             specificare semplicemente 'no' nel proprio "~/.tuxpaintrc":

               noprint=no
               uppercase=no

             Oppure, si possono usare opzioni simili agli argomenti di linea
             di comando descritti in seguito. Ad esempio:

               print=yes
               mixedcase=yes

     ----------------------------------------------------------------------

  Opzioni da linea di comando

           Le preferenze possono essere specificate anche nella linea di
           comando al momento di avviare Tux Paint.

             --fullscreen
             --800x600
             --nosound
             --noquit
             --noprint
             --printdelay=SECONDI
             --printcfg
             --simpleshapes
             --uppercase
             --grab
             --noshortcuts
             --nowheelmouse
             --nofancycursors
             --nooutlines
             --nostamps
             --nostampcontrols
             --mirrorstamps
             --keyboard
             --savedir CARTELLA
             --saveover
             --saveovernew
             --lang LINGUA
                     Queste attivano o corrispondono alle opzioni del file di
                     configurazione descritte sopra.

             --windowed
             --640x480
             --sound
             --quit
             --print
             --printdelay=0
             --noprintcfgv --complexshapes
             --mixedcase
             --dontgrab
             --shortcuts
             --wheelmouse
             --fancycursors
             --outlines
             --stamps
             --stampcontrols
             --dontmirrorstamps
             --mouse
             --saveoverask
                     Queste possono essere usate per sovrascrivere quelle
                     definite nel file di configurazione (se l'opzione non e
                     definita nel file, non e necessario sovrascriverla).

             --locale locale

                     Utilizza Run Tux Paint in uno dei linguaggi supportati.
                     Si veda la sezione "Scegliere una lingua differente".

                     (Se la localit`a e gi`a definita, ad esempio con una
                     variabile d'ambiente "$LANG", questa opzione non e
                     necessaria, dato che Tux Paint segue i settaggi
                     dell'ambiente, se possibile)

             --nosysconfig

                     Su GNU Linux e Unix, questa opzione evita la lettura del
                     file "/etc/tuxpaint/tuxpaint.conf".

                     Se esiste, verr`a usato solo il file di configurazione
                     "~/.tuxpaintrc".

     ----------------------------------------------------------------------

  Opzioni descrittive da linea di comando

           Le opzioni seguenti visualizzano del testo informativo sullo
           schermo. Il programma vero e proprio non viene avviato.

             --version
                     Mostra il numero della versione e la data relativi al
                     programma che si sta usando. Visualizza anche, se
                     presenti, le opzioni specificate durante la compilazione
                     (Si veda INSTALL.it.txt e FAQ.it.txt).

             --copying
                     Mostra un introduzione ai termini della licenza di
                     distribuzione di Tux Paint.

             --usage
                     Mostra la lista di argomenti disponibili da linea di
                     comando.

             --help
                     Mostra un testo di aiuto sull'uso di Tux Paint.

     ----------------------------------------------------------------------

  Scegliere una lingua differente

           Tux Paint e stato tradotto in molte lingue. Per accedere alle
           traduzioni si puo usare l'argomento "--lang" da linea di comando
           (ad esempio "--lang italian") oppure specificare l'opzione "lang="
           nel file di configurazione (ad esempio, "lang=italian").

           Tux Paint inoltre segue i settaggi del sistema operativo, se e
           gi`a settata una variabile d'ambiente relativa alla lingua il
           programma verr`a inizialmente visualizzato in quella lingua. (e
           possibile sovrascrivere questo comportamento usando l'argomento
           "--locale" da riga di comando (vedi sopra)).

           Di seguito l'elenco delle lingue supportate:

             +--------------------------------------------------------------+
             |Codice Locale  |Lingua             |Lingua                    |
             |               |(nome originale)   |(nome inglese)            |
             |---------------+-------------------+--------------------------|
             |C              |                   |English                   |
             |---------------+-------------------+--------------------------|
             |ca_ES          |Catalan            |Catal`a                   |
             |---------------+-------------------+--------------------------|
             |cs_CZ          |Cesky              |Czech                     |
             |---------------+-------------------+--------------------------|
             |da_DK          |Dansk              |Danish                    |
             |---------------+-------------------+--------------------------|
             |de_DE@euro     |Deutsch            |German                    |
             |---------------+-------------------+--------------------------|
             |el_GR.UTF8 (*) |                   |Greek                     |
             |---------------+-------------------+--------------------------|
             |en_GB          |                   |British English           |
             |---------------+-------------------+--------------------------|
             |es_ES@euro     |Espanol            |Spanish                   |
             |---------------+-------------------+--------------------------|
             |fi_FI@euro     |Suomi              |Finnish                   |
             |---------------+-------------------+--------------------------|
             |fr_FR@euro     |Franc,ais          |French                    |
             |---------------+-------------------+--------------------------|
             |he_IL (*)      |                   |Hebrew                    |
             |---------------+-------------------+--------------------------|
             |hu_HU          |Magyar             |Hungarian                 |
             |---------------+-------------------+--------------------------|
             |id_ID          |Bahasa Indonesia   |Indonesian                |
             |---------------+-------------------+--------------------------|
             |is_IS          |Islenska           |Icelandic                 |
             |---------------+-------------------+--------------------------|
             |it_IT@euro     |Italiano           |Italian                   |
             |---------------+-------------------+--------------------------|
             |ja_JP.UTF-8 (*)|                   |Japanese                  |
             |---------------+-------------------+--------------------------|
             |ko_KR.UTF-8 (*)|                   |Korean                    |
             |---------------+-------------------+--------------------------|
             |lt_LT.UTF-8    |Lietuviu           |Lithuanian                |
             |---------------+-------------------+--------------------------|
             |ms_MY          |                   |Malay                     |
             |---------------+-------------------+--------------------------|
             |nn_NO          |Norsk (nynorsk)    |Norwegian Nynorsk         |
             |---------------+-------------------+--------------------------|
             |nl_NL@euro     |                   |Dutch                     |
             |---------------+-------------------+--------------------------|
             |pl_PL          |Polski             |Polish                    |
             |---------------+-------------------+--------------------------|
             |pt_BR          |Portuges Brazileiro|Brazilian Portuguese      |
             |---------------+-------------------+--------------------------|
             |pt_PT          |Portuges           |Portuguese                |
             |---------------+-------------------+--------------------------|
             |ro_RO          |                   |Romanian                  |
             |---------------+-------------------+--------------------------|
             |ru_RU          |                   |Russian                   |
             |---------------+-------------------+--------------------------|
             |sk_SK          |Slovak             |                          |
             |---------------+-------------------+--------------------------|
             |sv_SE@euro     |Svenska            |Swedish                   |
             |---------------+-------------------+--------------------------|
             |tr_TR@euro     |                   |Turkish                   |
             |---------------+-------------------+--------------------------|
             |wa_BE@euro     |                   |Walloon                   |
             |---------------+-------------------+--------------------------|
             |zh_CN (*)      |                   |Chinese (Simplified)      |
             +--------------------------------------------------------------+

             (*) - Questi linguaggi richiedono il loro set di caratteri, in
             quanto non sono rappresentabili con un set "latino" come gli
             altri. Si veda la sezione Caratteri Speciali, di seguito.

    Specificare la lingua di sistema

             Modificare la lingua locale avr`a molti effetti sul sistema.

             Come anticipato, oltre a permettere la scelta della lingua
             tramite argomenti da linea di comando ("--lang" e "--locale"),
             Tux Paint segue le preferenze di lingua del sistema.

             Se non si e gi`a specificata la variabile di lingua nel sistema,
             la sezione seguente spiega come fare:

      Utenti GNU Linux/Unix

               Per prima cosa, assicurarsi che la lingua che si desidera
               usare sia attivata aprendo il file "/etc/locale.gen" e aprire
               il programma "locale-gen" da root.

               Nota: gli utenti Debian potrebbero essere in grado di usare
               semplicemente il programma "dpkg-reconfigure locales".

               Prima di aprire Tux Paint, definite la variabile locale
               "$LANG" con uno dei valori scritti sopra. (se volete che tutti
               i programmi che possono essere visualizzati nella lingua
               scelta vengano tradotti, potete inserire il comando seguente
               nello script di login del sistema, ad esempio ~/.profile,
               ~/.bashrc, ~/.cshrc, ecc.)

               Ad esempio, in una Bourne Shell (come BASH):

                 export LANG=it_IT@euro ; \
                 tuxpaint

               And in una C Shell (come TCSH):

                 setenv LANG it_IT@euro ; \
                 tuxpaint

     ----------------------------------------------------------------------

      Utenti Windows

               Tux Paint riconosce e utilizza automaticamente la lingua
               locale del sistema, quindi questa sezione e dedicata a chi
               intende provare lingue differenti.

               La cosa piu semplice da fare e usare l'argomento '--lang' nel
               collegamento (si veda "INSTALL.it.txt"). In ogni caso, usando
               una finestra prompt di MSDOS, e anche possibile usare un
               comando come questo:

                 set LANG=es_ES@euro

               ...che modificher`a la lingua di sistema solo finche quella
               finestra di DOS rimarr`a aperta.

               Per qualcosa di piu duraturo, si puo aprire il file
               'autoexec.bat' usando il programma "sysedit":

        Windows 95/98

                1. Fare click su 'Start', selezionare 'Esegui...'.
                2. Scrivere "sysedit" nel box 'Apri:' (con o senza le
                   virgolette).
                3. Fare click su 'OK'.
                4. Trovate la finestra del file AUTOEXEC.BAT nel System
                   Configuration editor.
                5. Aggiungere al file, in fondo, quanto segue:

                     set LANG=es_ES@euro

                6. Chiudere System Configuration Editor, rispondendo Si alla
                   richiesta di salvataggio delle modifiche.
                7. Riavviare il computer.

                 Per far si che il cambiamento si ripercuota su tutto il
                 sistema e su tutte le applicazioni e possibile usare il
                 pannello di controllo "Opzioni Internazionali":

                1. Fare click su 'Start' button, selezionare 'Pannello di
                   Controllo'.
                2. Fare doppio click su "Opzioni Internazionali".
                3. Selezionare una lingua dalla lista.
                4. Fare click su 'OK'.
                5. Riavviare il computer quando richiesto.

    Caratteri speciali

             Alcune lingue necessitano caratteri speciali. Servono quindi
             file di caratteri (in formato TrueType (TTF)), che sono troppo
             grandi per essere inseriti nell'installazione di Tux Paint e
             sono disponibili separatamente. (si veda la tabella sopra, alla
             voce "Scegliere una lingua differente")

             Quando si avvia Tux Paint in una lingua che necessita di un
             apposito file di caratteri, Tux Paint cerca di caricare il file
             dalla cartella "fonts" (in una sottocartella "locale"). Il nome
             del file corrisponde per le prime due lettere al codice 'locale'
             della lingua. (ad esempio, "ko" per il Coreano, "ja" per il
             Giapponese).

             Ad esempio, su GNU Linux o Unix, quando Tux Paint viene eseguito
             in Coreano (ad esempio con l'opzione "--lang korean"), Tux Paint
             cerca di caricare il seguente file::

               /usr/share/tuxpaint/fonts/locale/ko.ttf

             e possibile scaricare i file dei caratteri per le lingue
             supportate dal sito di Tux Paint
             http://www.newbreedsoftware.com/tuxpaint/. (Guardare nella
             sezione 'Fonts' sotto 'Download.')

             Su Unix e GNU Linux, e possibile usare il Makefile disponibile
             con il file carattere per installare il carattere nella
             posizione appropriata.

     ----------------------------------------------------------------------

Schermata iniziale

         Quando Tux Paint viene aperto, appare un'immagine con il nome del
         programma e alcune informazioni sugli autori.

                               [Title Screenshot]

         Quando il caricamento e completato, fare click oppure premere un
         tasto per continuare (l'immagine scomparir`a da se dopo 30 secondi).

     ----------------------------------------------------------------------

Schermata principale

         La schermata principale e divisa nelle seguenti sezioni:

         Lato sinistro: Barra degli Strumenti (Toolbar)

                 Contiene gli strumenti per disegnare e per gestire i file.

[Tools: Paint, Stamp, Lines, Shapes, Text, Magic, Undo, Redo,       Eraser, New,
                            Open, Save, Print, Quit]

         In mezzo: spazio per disegnare (Drawing Canvas)

                 e la parte piu grande dello schermo. e, ovviamente, il posto
                 in cui si disegna!

                                   [(Canvas)]

         Lato destro: Selezione (Selector)

                 A seconda dello strumento corrente, la Selezione mostra cose
                 differenti. Ad esempio, quando viene selezionato il
                 Pennello, mostra i diversi tipi di pennelli disponibili.
                 Quando viene selezionato il Timbro, mostra le diverse forme
                 a disposizione.

                 [Selectors - Brushes, Letters, Shapes, Stamps]

         In basso: Colori (Colors)

                 La tavolozza dei colori disponibili.

  [Colors - Black, White, Red, Pink, Orange, Yellow, Green, Cyan,       Blue,
                              Purple, Brown, Grey]

         In basso: Area di aiuto (Help Area)

                 Tux, il pinguino mascotte di Linux, mostra simpatiche
                 infrormazioni durante l'uso degli strumenti.

 (For example: 'Pick a shape. Click to pick the center, drag, then       let go
when it is the size you want.  Move around to rotate it, and       click to draw
                                      it.)

     ----------------------------------------------------------------------

Strumenti

  Strumenti di disegno

           Pennello

                   Il Pennello permette di disegnare a mano libera,
                   utilizzando vari tipi di pennelli (da scegliere dalla
                   Selezione sulla destra) e i colori della tavolozza (in
                   basso).

                   Fare click e muovere il mouse per disegnare.

                   Mentre si disegna, viene eseguito un suono. Piu grande e
                   il pennello, piu profondo e il suono.

     ----------------------------------------------------------------------

           Timbro

                   Il Timbro e simile a degli adesivi. Permette di incollare
                   disegni pronti o immagini fotografiche all'interno del
                   proprio disegno.

                   Quando si muove il mouse, una linea lo segue evidenziando
                   le proporizioni del disegno sullo spazio per disegnare.

                   Ad ogni timbro puo essere associato un suono. Alcuni
                   timbri possono essere colorati o cambiare colore.

                   Ogni Timbro puo essere modificato in altezza e larghezza,
                   e molti Timbri possono essere capovolti o specchiati,
                   usando i comandi in basso a destra sullo schermo.

                   NOTA: Se e stato attivata l'opzione "nostampcontrols", Tux
                   Paint non mostrer`a i comandi Specchia, Capovolgi e
                   Ingrandisci dei Timbri. Vedi sezione "Opzioni".

     ----------------------------------------------------------------------

           Linea

                   Questo strumento permette di disegnare linee dritte di
                   diverso spessore e colore..

                   Fare click e temere premuto per scegliere il punto di
                   partenza della linea. Muovendo il mouse, si crea una riga
                   sottile per evidenziare il modo in cui la linea sar`a
                   disegnata.

                   Lasciare il tasto del mouse per completare la linea. Si
                   udir`a un suono.

     ----------------------------------------------------------------------

           Forma

                   Questo strumento permette di disegnare alcune semplici
                   forme, perimetrali e piene.

                   Puoi selezionare una forma dal menu di destra.

                   Fare click e tenere premuto per ingrandire la forma.
                   Alcune forme (ad esempio il rettangolo e l'ovale) possono
                   cambiare proporzioni, altre (ad esempio il quadrato e il
                   cerchio) invece no.

                   Lasciare il tasto del mouse per selezionare la grandezza
                   della forma.

                        Modo "normale":

                                Ora, muovere il mouse per ruotare la forma.

                                Facendo nuovamente click col mouse, la forma
                                verr`a disegnata nel colore selezionato.

                        Modo "forme semplici":
                                Se questa modalit`a e stata attivata (ad
                                esempio con l'argomento da riga di comando
                                "--simpleshapes") la forma verr`a disegnata
                                immediatamente, senza essere ruotata.

     ----------------------------------------------------------------------

           Testo

                   Selezionare un carattere (dalle "Lettere" disponibili
                   sulla destra) e un colore (dalla tavolozza in basso). Fare
                   click sull'area di disegno per far apparire il cursore.
                   Ora si puo scrivere un testo con la tastiera.

                   Premere [Invio] o [Enter] per disegnare il testo sul
                   disegno. Il cursore si sposter`a sulla riga sottostante.

                   Fare click da un'altra parte nel disegno e il testo appena
                   scritto verr`a spostato in quel punto, dove e possibile
                   modificarlo.

     ----------------------------------------------------------------------

           Magia (Effetti speciali)

                   Lo strumento "Magia" non e altro che un insieme di altri
                   strumenti. Puoi selezionare l'effetto sulla destra e poi
                   fare click sul disegno per applicare l'effetto.

                        Arcobaleno
                                E simile al Pennello, ma quando si muove il
                                mouse, cambia tutti i colori dell'arcobaleno.

                        Scintille
                                Questo strumento disegna scintille gialle sul
                                disegno.

                        Specchia
                                Quando si fa click sul disegno, all'immagine
                                viene applicata una simmetria ad asse
                                verticale.

                        Capovolgi
                                E simile a "Specchia", ma applica una
                                simmetria ad asse orizzontale.

                        Sfuma
                                Questo strumento permette di sfumare il
                                disegno.

                        Blocchi
                                Facendo click col mouse e trascinandolo, si
                                rende l'immagine "pixellata", mischiando i
                                pixel di quell'area.

                        Negativo
                                Permette di invertire i colori (ad esempio il
                                bianco diventa nero, e viceversa).

                        Scolora
                                Permette di scolorire un'area del disegno
                                (usando lo strumento molte volte nello stesso
                                punto, questo diventer`a bianco).

                        Gesso
                                Permette di modificare il disegno facendolo
                                assomigliare ad un disegno fatto con il
                                gesso.

                        Gocciola
                                Questo strumento fa "gocciolare" i colori del
                                disegno.

                        Pesante
                                Fa prevalere il colore piu scuro.

                        Leggero
                                Fa prevalere il colore piu chiaro.

                        Riempi
                                Campisce un'area con il colore selezionato.
                                Permette di riempire velocemente ampie aree
                                del disegno, come se fosse un album da
                                colorare.

     ----------------------------------------------------------------------

           Gomma

                   Questo strumento permette di cancellare porzioni di
                   disegno. Basta fare click in un punto per farlo tornare
                   bianco.

                   Mentre si muove il mouse, il puntatore e seguito da una
                   grande linea quadrata, che mostra quale parte del disegno
                   verr`a cancellata facendo click.

                   Quando si cancella, viene eseguito un suono.

     ----------------------------------------------------------------------

  Comandi

           Annulla

                   Facendo click su questro comando e possibile annullare
                   l'ultima azione compiuta. E anche possibile annullare piu
                   di una volta!

                   Nota: premere [Control]-[Z] sulla tastiera avr`a lo stesso
                   effetto.

     ----------------------------------------------------------------------

           Ripeti

                   Facendo click su questo comando e possibile ripristinare
                   le modifiche appena cancellate con lo strumento "Annulla".

                   Finche non si disegna nuovamente, e possibile "ripetere"
                   tante volte quante si e "annullato"!

                   Nota: premere [Control]-[R] sulla tastiera avr`a lo stesso
                   effetto.

     ----------------------------------------------------------------------

           Nuovo

                   Fare click sul pulsante "Nuovo" per iniziare un nuovo
                   disegno. Prima di cancellare quello corrente, verr`a
                   chiesta una conferma.

                   Nota: premere [Control]-[N] sulla tastiera avr`a lo stesso
                   effetto.

     ----------------------------------------------------------------------

           Apri

                   Questo comando mostra l'elenco dei disegni salvati. Nel
                   caso ce ne siano piu di quanti possano essere contenuti in
                   una schermata, usare le freccie "Su" e "Giu" per scorrere
                   la lista.

                   Fare click su un disegno per selezionarlo, dopodiche...

                        * Fare click sul pulsante verde "Apri" per caricare
                          il disegno selezionato.

                          (In alternativa, e possibile fare doppio click su
                          un disegno per caricarlo.)

                        * Fare click sul pulsante marrone "Cancella" (il
                          bidone della spazzatura) per cancellare il disegno
                          selezionato (verr`a chiesta una conferma).

                        * Fare click sul pulsante rosso "Indietro" per
                          tornare al disegno corrente.

                   Se si sceglie di caricare un disegno senza aver salvato
                   quello corrente, verr`a chiesto se si lo si vuole salvare.
                   (Vedi "Salva")

                   Nota: premere [Control]-[O] sulla tastiera avr`a lo stesso
                   effetto.

     ----------------------------------------------------------------------

           Salva

                   Salva il disegno corrente.

                   Se non era mai stato salvato prima, sar`a aggiunto un
                   elemento alla lista dei disegni salvati (ovvero: verr`a
                   creato un nuovo file).

                   Nota:non viene chiesto nulla (nemmeno un nome da dare al
                   file). Semplicemente, il disegno viene salvato e viene
                   eseguito un suono.

                   Se il disegno e gi`a stato salvato in precedenza, o se il
                   disegno e stato caricato con il comando "Apri", verr`a
                   prima chiesto se sovrascrivere la vecchia versione o
                   creare un nuovo disegno (un nuovo file).

                   (NOTA: se l'opzione "saveover" o "saveovernew" e attiva,
                   non verr`a chiesto nulla prima di sovrascrivere. Si veda
                   la sezione "Opzioni")

                   Nota: premere [Control]-[S] sulla tastiera avr`a lo stesso
                   effetto.

     ----------------------------------------------------------------------

           Stampa

                   Facendo click su questo pulsante per stampare il disegno!

                        Disabilitare la stampa

                                Se e stata attivata l'opzione "noprint"
                                (attraverso l'uso di "noprint=yes" nel file
                                di configurazione di Tux Paint o usando
                                l'argomento da riga di comando "--noprint"),
                                il pulsante "Stampa" sar`a disabilitato.

                                Si veda la sezione "Opzioni".

                        Limitare la stampa

                                Se e stata attivata l'opzione "printdelay"
                                (attraverso l'uso di "printdelay=SECONDI" nel
                                file di configurazione o usando l'argomento
                                da riga di comando "--printdelay=SECONDI"), e
                                possibile stampare solo ogni SECONDI secondi.

                                Ad esempio, usando "printdelay=60", sar`a
                                possibile stampare solo una volta ogni
                                minuto.

                                Si veda la sezione "Opzioni".

                        Comandi di stampa

                                (solo per GNU Linux e Unix)

                                Il comando usato per stampare e in realt`a
                                una lista di comandi che convertono una
                                immagine PNG in un file PostScript e lo
                                mandano alla stampante:

                                  pngtopnm | pnmtops | lpr

                                Questo comando puo essere modificato usando
                                l'opzione "printcommand" nel file di
                                configurazione di Tux Paint.

                                Si veda la sezione "Opzioni".

                        Opzioni di stampa

                                (solo per Windows)

                                Inizialmente Tux Paint utilizza la stampante
                                prefedinita e opzioni standard quando viene
                                usato il pulsante "Stampa".

                                Comunque, se si preme il tasto [ALT] mentre
                                si fa click sul pulsante, se non si e in
                                modalit`a tutto schermo, appare una finestra
                                di dialogo di stampa di Windows, dove e
                                possibile modificare le opzioni di stampa.

                                E possibile far apparire questa finestra di
                                dialogo ad ogni stampa con l'opzione
                                "printcfg", usando l'argomento da riga di
                                comando "--printcfg" o "printcfg=yes" nel
                                file di configurazione di Tux Paint
                                ("tuxpaint.cfg").

                                Se l'opzione "printcfg" e attiva, le opzioni
                                di stampa saranno caricate dal file
                                "userdata/print.cfg". Ogni cambiamento verr`a
                                salvato nello stesso file.

                                Si veda la sezione "Opzioni".

     ----------------------------------------------------------------------

           Esci

                   Facendo click sul pulsante "Esci" chiudendo la finestra di
                   Tux Paint o premendo il tasto "Escape" sulla tastiera,
                   Tux Paint verr`a terminato.

                   (NOTA: il pulsante "Esci" puo essere disabilitato (ad
                   esempio con l'argomento da riga di comando "--noquit"), ma
                   il tasto [Escape] funzioner`a ancora. Si veda la sezione
                   "Opzioni")

                   Verr`a chiesta una conferma della volont`a di terminare il
                   programma.

                   Se si sceglie di uscire, e non si e ancora salvato il
                   disegno, verr`a chiesto se si desidera salvarlo. Se non e
                   una nuova immagine verr`a chiesto se si vuole
                   sovrascrivere la vecchia versione o creare un nuovo file
                   (Si veda "Salva").

                   NOTA: Se l'immagine viene salvata, verr`a ricaricata
                   automaticamente al prossimo avvio di Tux Paint!

     ----------------------------------------------------------------------

                      Caricare altre immagini in Tux Paint

       Dato che la finestra "Apri" di Tux Paint mostra solo le immagini
       create con Tux Paint, cosa succederebbe se si volessero caricare altre
       immagini e disegni per modificarle con Tux Paint?

       Per farlo, e sufficiente convertire l'immagine in formato PNG
       (Portable Network Graphic) e inserirla nella cartella dei file salvati
       di Tux Paint ("~/.tuxpaint/saved/" su GNU Linux e Unix,
       "userdata\saved\" su Windows, "Library/Preferences/tuxpaint/saved/" su
       Mac OS X).

Usare 'tuxpaint-import'

         Gli utenti GNU Linux e Unix possono usare lo script di shell
         "tuxpaint-import" che viene installato insieme a Tux Paint. Utilizza
         alcuni strumenti NetPBM per convertire l'immagine ("anytopnm"),
         scalarla in modo che possa essere contenuta nell'area di disegno di
         Tux Paint ("pnmscale") e riconvertirla in PNG ("pnmtopng").

         Utilizza anche il comando "date" per prelevare data e ora correnti,
         che costituiscono il formato convenzionale che Tux Paint utilizza
         per i nomi dei file salvati (infatti non viene mai chiesto
         all'utente di scegliere un nome di file quando Salva o Apre un
         disegno!)

         Per usare 'tuxpaint-import', e sufficiente dare il comando da un
         propt di linea di comando e inserire il nome del (o dei) file da
         convertire.

         Verranno convertiti e inseriti nella cartella appropriata (Nota: se
         si sta eseguendo il comando per conto di un utente diverso - ad
         esempio il proprio figlio - e necessario accertarsi di stare usando
         il suo account).

         Esempio:

           $ tuxpaint-import grandma.jpg
           grandma.jpg -> /home/username/.tuxpaint/saved/20020921123456.png
           jpegtopnm: WRITING A PPM FILE

         La prima riga ("tuxpaint-import grandma.jpg") e il comando da dare.
         Le altre due sono l'output del programma mentre lavora.

         Adesso e possibile far partire Tux Paint, e una versione di
         quell'immagine sar`a disponibile all'interno della finestra "Apri".
         Per aprirla basta fare doppio click sulla sua icona!

Procedura manuale

         Gli utenti Windows, Mac OS X e BeOS attualmente devono effettuare la
         conversione a mano.

         Aprire un programma di grafica che sia in grado di gestire sia le
         immagini che volete inserire sia immagini in formato PNG (si veda il
         file "PNG.it.txt" per una lista di software suggeriti e per altre
         informazioni).

         Ridurre l'immagine ad una larghezza che non superi i 448 pixel e
         un'altezza che non superi i 376 pixel (la grandezza massima deve
         essere di 448 x 376 pixel).

         Salvare l'immagine in formato PNG. E fortemente consigliato usare un
         nome di file composto da data e ora corrente, dato che questa e la
         convenzione usata da Tux Paint:

           AAAAMMGGOOmmss.png

           * AAAA = Anno
           * MM = Mese (01-12)
           * GG = Giorno (01-31)
           * OO = Ora, in formato 24-ore (00-23)
           * mm = Minuti (00-59)
           * ss = Secondi (00-59)

         esempio:

           20020921130500 = 21 settembre 2002, 13:05:00

         Inserire l'immagine PNG nella cartella dei file salvati di
         Tux Paint. Su Windows, si trova nella cartella "userdata". Su Mac OS
         X, nella cartella "Library/Preferences/tuxpaint/" all'interno della
         cartella home.

     ----------------------------------------------------------------------

                              Estendere Tux Paint

       Se si desidera cambiare cose come i Pennelli e i Timbri usati da
       Tux Paint, e possibile farlo in modo abbastanza semplice aggiungendo o
       togliendo file dal vostro disco fisso.

       Nota: e necessario riavviare Tux Paint perche le modifiche abbiano
       effetto.

Dove vanno i file

  File Standard

           Tux Paint cerca i suoi numerosi file di funzionamento nella
           cartella "data".

    GNU Linux e Unix

             La locazione di questa cartella dipende dal valore settato per
             la variabile "DATA_PREFIX" al momento della compilazione. Si
             veda INSTALL.it.txt per maggiori informazioni.

             In ogni caso, la locazione predefinita di questa cartella e:

               /usr/local/share/tuxpaint/

             Nel caso si sia installato un pacchetto, la locazione sar`a piu
             probabilmente:

               /usr/share/tuxpaint/

    Windows

             Tux Paint cerca una cartella "data" nella stessa cartella in cui
             si trova l'eseguibile. Questa e la cartella che e stata usata al
             momento dell'installazione, ad esempio:

               C:\Programmi\TuxPaint\data

    Mac OS X

             Tux Paint salva i file nella cartella "Libraries" del vostro
             account, sotto "Preferences", ad esempio:

               /Users/Joe/Library/Preferences/

     ----------------------------------------------------------------------

  File personali

           E anche possibile creare pennelli, timbri e caratteri nella
           propria cartella personale per permettere a Tux Paint di trovarli.

    GNU Linux e Unix

             La propria cartella personale di Tux Paint e "~/.tuxpaint/".

             Quindi, se la vostra cartella home e "/home/pippo", allora la
             cartella di Tux Paint e "/home/pippo/.tuxpaint/".

             Non dimenticare il punto (".") prima del "tuxpaint"!

    Windows

             La propria cartella personale di Tux Paint e chiamata "userdata"
             e si trova nella stessa cartella dell'eseguibile, ad esempio:

               C:\Programmi\TuxPaint\userdata

           Per aggiungere pennelli, timbri e caratteri, creare una
           sottocartella all'interno della propria cartela di Tux Paint,
           rispettivamente di nome "brushes", "stamps" e "fonts".

           (Ad esempio, se si crea un pennello di nome "flower.png", su GNU
           Linux o Unix andr`a messo nella cartella "~/.tuxpaint/brushes/".)

     ----------------------------------------------------------------------

Pennelli

         I pennelli usati per disegnare con gli strumenti "Pennello" e
         "Linea" in Tux Paint sono semplici immagini PNG in scala di grigi.

         L'alpha (trasparenza) dell'immagine PNG viene usata per determinare
         la forma del pennello, questo significa che la forma puo avere un
         effetto "anti-alising" e perfino essere parzialmente trasparente!

         Le immagini dei pennelli non devono superare i 40 pixel di larghezza
         e i 40 pixel di altezza (quindi la grandezza massima deve essere
         40 x 40 pixel).

         Vanno semplicemente inserite nella cartella "brushes".

         Nota: se i nuovi pennelli risultano essere quadrati o rettangoli
         pieni, e perche hai dimenticato di usare la trasparenza alpha! Vedi
         il file di documentazione PNG.it.txt per maggiori informazioni su
         questo.

     ----------------------------------------------------------------------

Timbri

         Tutte le informazioni relative ai Timbri vanno nella cartella
         "stamps". E utile creare sottocartelle per organizzare i timbri (ad
         esempio, e possibile avere una cartella "festivit`a" che contiene le
         sottocartelle "natale" e "pasqua").

  Immagine

           I timbri possono essere composti da alcuni file separati. L'unico
           obbligatorio, ovviamente, e l'immagine del timbro.

           I timbri usati da Tux Paint sono immagini PNG. Possono essere a
           colori o in scala di grigi. L'alpha (trasparenza) del file PNG e
           usato per determinare i contorni dell'immagine (altrimenti
           verrebbero dei grossi rettangoli sul disegno).

           La grandezza dei timbri non ha limiti, ma in pratica un'immagine
           larga 100 pixel e alta 100 pixel (100 x 100 pixel) e gi`a grande
           per Tux Paint.

           Nota: se i nuovi timbri hanno un contorno quadrato o rettangolare
           di un colore fisso (ad esempio, bianco o nero) e perche hai
           dimenticato di usare la trasparenza alpha! Vedi il file di
           documentazione PNG.it.txt per maggiori informazioni su questo.

     ----------------------------------------------------------------------

  Testo descrittivo

           Un file di testo (".TXT") con lo stesso nome dell'immagine PNG (ad
           esempio, la descrizione di "picture.png" e contenuta nel file
           "picture.txt" nella stessa cartella).

           La prima riga del file sar`a usata come testo descrittivo
           predefinito del timbro.

    Supporto multilingue

             Al file possono essere aggiunte altre righe per rendere
             disponibili le traduzioni della descrizione, in modo tale che
             vengano mostrate quando Tux Paint viene usato in un'altra lingua
             (come Francese o Spagnolo).

             Ci sono tre modi per inserire le traduzioni delle descrizioni di
             un timbro in un file ".txt". In ciascun caso, l'inizio della
             riga deve corrispondere al Codice Locale della lingua in
             questione (ad esempio, "de" per tedesco, "fr" per francese).

               * Le righe che iniziano con "xx=" (dove "xx" e un Codice
                 Locale) vengono considerate ASCII semplice. Di conseguenza,
                 ogni carattere speciale presente nella riga verr`a
                 interpretato letteralmente.

                 Per esempio, "es=*Ni*os!", verr`a reso letteralmente come
                 "*Ni*os!"

               * Le righe che iniziano con "xx.esc=" possono contenere
                 sequenze di escape che permettono di creare descrizioni
                 usando caratteri ASCII speciali (come "*" e "*") senza
                 bisogno di scervellarsi per capire come ottenere simili
                 caratteri con qualsiasi editor si stia usando.

                 Le sequenze di escape sono identiche a quelle usate in HTML
                 per mostrare i caratteri ASCII compresi tra 161 e 255. Una
                 sequenza comincia con "&" (e commerciale), e finisce con ";"
                 (punto e virgola). Si veda il file ESCAPES.it.txt per la
                 lista delle sequenze di escape supportate.

                 Ad esempio, "es.esc=&iexcl;Ni&ntilde;os!" diventa "*Ni*os!"

                 Nota: come in HTML, se si desidera scrivere la "e
                 commerciale" ("&") nelle descrizioni che usano il metodo
                 "xx.esc", e necessario utilizzare la sua sequenza di escape:
                 "&amp;".

               * Le righe che iniziano con "xx.utf8=" possono essere usate
                 per usare testo in formato UTF-8 all'interno delle
                 traduzioni delle descrizioni. In questo caso e necessario
                 usare un editor in grado di salvare file in formato UTF-8.

             Se non ci sono traduzioni disponibili per la lingua con cui si
             sta usando Tux Paint, viene visualizzata la riga predefinita
             (ovvero la prima, che solitamente e in inglese).

    Utenti Windows

             Usare NotePad o WordPad per modificare/creare questi file.
             Assicurarsi di salvarli in testo semplice, e che il loro nome
             termini con l'estensione ".txt"...

     ----------------------------------------------------------------------

  Effetto sonoro

           Un file in formato WAVE (".WAV") con lo stesso nome del file PNG
           che costituisce l'immagine del timbro. (ad esempio, il suono
           associato a "picture.png" e il file "picture.wav" che si trova
           nella stessa cartella).

    Supporto multilingue

             Per suoni in differenti lingue (ad esempio, se e il suono di
             qualcuno che pronuncia una parola, e si vuole tradurre il
             significato della parola detta), bisogna usare un file WAVE con
             il Codice Locale nel nome del file, nella forma:
             "TIMBRO_CODICE.wav"

             Il suono associato al timbro "picture.png", quando Tux Paint
             viene usato in lingua spagnola, sarebbe "picture_es.wav". In
             francese sarebbe "picture_fr.wav". E cos`i via...

             Se non e presente un file relativo alla lingua in uso, Tux Paint
             user`a quello predefinito (ad esempio, "picture.wav").

     ----------------------------------------------------------------------

  Opzioni dei timbri

           Oltre che una forma grafica, una descrizione testuale e un effetto
           sonoro, un timbro puo avere anche altre propriet`a. Per usarle,
           bisogna creare il "data file" del timbro.

           Un data file e semplicemente un file di testo contenente le
           opzioni.

           Il file ha lo stesso nome dell'immagine PNG e l'estensione ".dat"
           (ad esempio il data file relativo al timbro "picture.png" e il
           file "picture.dat" contenuto nella stessa cartella).

    Timbri colorati

             I timbri possono essere "a colori" o "a tinta".

      A colori

               I timbri "a colori" sono simili ai pennelli - si sceglie il
               timbro desiderato e si seleziona il colore con cui lo si vuole
               disegnare (i timbri dei simboli, come i segni matematici e
               musicali, ne sono un esempio).

               Dell'immagine originale non viene usato altro che la
               trasparenza (il canale "alpha"). Il colore dei timbri risulta
               senza sfumature.

               Aggiungere la parola "colorable" al data file del timbro.

      A tinta

               I timbri "a tinta" sono simili a quelli "a colori", ma vengono
               mantenuti i dettagli dell'immagine originale (tecnicamente,
               viene usata l'immagine originale, ma la tonalit`a viene
               cambiata in base al colore selezionato).

               Aggiungere la parola "tintable" al data file del timbro.

    Timbri non modificabili

             Normalmente un timbro puo essere capovolto, specchiato, o
             entrambe le cose. Queste modifiche vengono fatte usando i
             pulsanti sotto la "selezione", in basso a destra nella schermata
             di Tux Paint.

             A volte non ha senso permettere che un timbro venga capovolto o
             specchiato, ad esempio con i timbri di numeri e lettere. Altre
             volte i timbri sono simmetrici, quindi capovolgere o specchiare
             il timbro non serve.

             Per fare in modo che un timbro non possa essere capovolto,
             aggiungere la parola "noflip" al data file del timbro.

             Per fare in modo che un timbro non possa essere specchiato,
             aggiungere la parola "nomirror" al data file del timbro.

    Utenti Windows

             Usare NotePad o WordPad per modificare/creare questi file.
             Assicurarsi di salvarli in testo semplice, e che il loro nome
             termini con l'estensione ".dat" e non con ".txt"...

  Immagini pre-specchiate

           In alcuni casi, puo essere utile creare una versione specchiata
           del timbro. Ad esempio, pensando ad un camion dei pomperi con la
           scritta "Vigili del fuoco" sulla fiancata, probabilmente non si
           desidera che la scritta appaia al contrario quando il timbro viene
           specchiato!

           Per creare un'immagine specchiata che Tux Paint usi al posto di
           specchiare direttamente il timbro, basta creare una seconda
           immagine ".png" con lo stesso nome, ma che termini con "_mirror"
           prima dell'estensione.

           Ad esempio, per il timbro "CamionVdF.png" si crei un file chiamato
           "CamionVdF_mirror.png", che verr`a usato quando il timbro viene
           specchiato (evitando l'uso dell'immagine "CamionVdF.png" al
           contrario).

     ----------------------------------------------------------------------

Caratteri

         I caratteri usati da Tux Paint sono TrueType Fonts (TTF).

         E sufficiente inserire i file nella cartella "fonts" perche
         Tux Paint li carichi e permetta di utilizzarli con lo strumento
         "Testo", mettendoli a disposizione in quattro grandezze
         selezionabili dal menu sulla destra.

     ----------------------------------------------------------------------

                               Altre Informazioni

       Per maggiori inormazioni, si vedano gli altri file di documentazione
       allegati a Tux Paint.

       Se si desidera aiuto, e possibile contattare liberamente New Breed
       Software:

         http://www.newbreedsoftware.com/

       E anche possibile partecipare alle numerose mailing list di Tux Paint:

         http://www.newbreedsoftware.com/tuxpaint/lists/
