                                   Tux Paint

                        Een tekenprogramma voor kinderen

                         Copyright 2002, Bill Kendrick
                               New Breed Software

                           bill@newbreedsoftware.com
                   http://www.newbreedsoftware.com/tuxpaint/

                        14 Juni 2002 - 16 november 2002

     ----------------------------------------------------------------------

                                      Over

     "Tux Paint" is een tekenprogramma voor kinderen. het is eenvoudig te
     bedienen en heeft een vaste venster grootte. Het programma geeft toegang
     tot eerder gemaakte tekeningen d.m.v. een 'thumbnail browser' en hiermee
     geen toegang tot het onderliggende bestandssysteem.

     In vergelijking met populaire tekenprogramma's als "De GIMP," heeft
     TuxPaint een beperkt aantal mogelijkheden. Het is echter veel
     eenvoudiger te bedienen en heeft kind-vriendelijke features als leuke
     geluidseffecten.

     ----------------------------------------------------------------------

                                   Licentie:

     Tux Paint is een Open Source project, Vrije Software uitgegeven onder de
     GNU General Public License (GPL). De software is vrij en de broncode van
     het programma is vrij beschikbaar. (Dit maakt het voor andere gebruikers
     mogelijk om het programma aan hun wensen aan te passen, fouten eruit te
     halen en delen van het programma te gebruiken in hun eigen programma's,
     onder de GPL licentie.)

     Voor volledige informatie over de GPL licentie leest u de COPYING.txt
     na.

     ----------------------------------------------------------------------

                                    Doelen:

   TuxPaint moet een gemakkelijk en vooral leuk tekenprogramma voor kinderen
   zijn.
           Tux Paint is een eenvoudig tekenprogramma voor kinderen. De
           achterliggende gedachte is niet om een algemeen tekenprogramma te
           maken. Het moet vooral leuk en gemakkelijk te bedienen zijn.
           Geluidseffecten en een 'stripfiguur' helpen de gebruiker en zorgen
           voor een beetje vermaak. Het programma beschikt ook over een
           aantal grappige muis-aanwijzers.

   Uitbreidbaarheid
           Tux Paint is uitbreidbaar. Kwasten en stempels kunnen naar
           believen toegevoegd, maar ook verwijderd, worden Zo zou een leraar
           een verzameling afbeeldingen van dieren kunnen invoegen om de
           kinderen hiermee een tekening te laten maken als ondersteuning bij
           andere lessen. Elke vorm kan zijn eigen geluidje hebben maar ook
           aanwijzingen in tekst-vorm zijn mogelijk wanneer het kind de vorm
           kiest.

   Andere operating systemen
           Tux Paint is beschikbaar op een veelvoud van operating systemen:
           Windows, Linux, etc. Het gebruikersinterface blijft hetzelfde.
           Tux Paint werkt ook op oudere computers (zoals een Pentium 133) en
           kan aangepast worden om te werken op oudere systemen.

   Eenvoud
           Tuxpaint geeft alleen toegang tot de tekeningen gemaakt met het
           programma zelf. Men hoeft zich dus geen zorgen te maken dat
           bestanden gewist worden of kwijt raken. De gemaakte tekening wordt
           automatisch opgeslagen als het programma afgesloten wordt, en
           verschijnt weer als het programma opnieuw opgestart wordt. Bij het
           opslaan van een afbeelding wordt niet om een naam gevraagd, maar
           gebeurt dit door op een thumbnail te klikken. De kinderen hoeven
           dus niet te kunnen lezen!

   --------------------------------------------------------------------------

                              Andere documentatie

     Andere documentatie bij Tux Paint (in de "docs" map/directory):

     *   AUTHORS.txt
         Een lijst van de auteurs en medewerkers.

     *   CHANGES.txt
         Een samenvatting van de veranderingen bij de verschillende uitgaven
         van het programma.

     *   COPYING.txt
         De licentievoorwaarden (GPL).

     *   INSTALL.txt
         Instructie voor het installeren / compileren van Tux Paint, zover
         beschikbaar.

     *   PNG.txt
         Aanwijzingen voor het maken van afbeeldingen in het PNG formaat.

     *   README.txt
         Dit bestand.

     *   TODO.txt
         Een overzicht van fouten en zaken die nog aangepast moeten worden

     ----------------------------------------------------------------------

                           Het gebruik van Tux Paint

Tux Paint installeren

     Voor aanwijzingen zie de INSTALL.txt file.

     ----------------------------------------------------------------------

Tux Paint starten

  Linux/Unix gebruikers

     Typ het volgende op de commando-regel ( "$"):

     $ tuxpaint

     Men kan Tux Paint ook toevoegen aan het menu of een icoon op het
     bureaublad plaatsen (bijvoorbeeld in GNOME of KDE). Kijk hiervoor in de
     documentatie van uw desktop...

     Treden er fouten op dan zullen deze verschijnen op het console (in
     "stderr").

     ----------------------------------------------------------------------

  Windows gebruikers

                                                           [Icon]             
                                                          Tux Paint           

     Klik op het "Tux Paint" icoon op de desktop of dubbel-klik op het
     "tuxpaint.exe" icoon in de 'Tux Paint' map op uw computer.

     Mochten er fouten optreden dan worden deze opgeslagen in het bestand
     "stderr.txt" in de Tux Paint map.

     Zie "INSTALL.txt" voor meer details hoe u de eigenschappen van het icoon
     kunt aanpassen. In Tux Paint, kunt u opties hieraan toe voegen. (d.m.v
     de commando-regel).

     Om Tux Paint extra opties mee te geven moet u "tuxpaint.exe" vanuit een
     MSDOS Prompt venster starten. (Zie "INSTALL.txt" voor meer details.)

Opties

  Configuratie File

     Op eenvoudige wijze kunt u een configuratie file maken voor Tux Paint,
     deze file wordt elke keer bij het opstarten gelezen.

     Dit bestand is een eenvoudig tekstbestand welke de opties bevat die u
     wilt opstarten.

    Linux gebruikers

     Het bestand dat u moet aanmaken heeft de naam ".tuxpaintrc" en moet
     geplaatst worden in uw home directory. (bijvoorbeeld "~/.tuxpaintrc" of
     "$HOME/.tuxpaintrc")

     De instellingen in de .tuxpaintrc overschrijven de instellingen gemaakt
     in de systeem-wijde configuratie file. In de systeem configuratie file
     worden standaard geen instellingen gedaan. Deze file is te vinden in de
     volgende map:

     /etc/tuxpaint/tuxpaint.conf

     De instellingen die gedaan worden in beide files kunnen op hun beurt
     weer overschreven worden door opties mee te geven op de commando-regel.
     Of u geeft op de commando-regel de volgende optie mee:

     --nosysconfig

    Windows gebruikers

     De file die u moet aanmaken heeft de naam "tuxpaint.cfg" en moet
     geplaatst zijn in de Tux Paint's map.

     Om deze file aan te maken kunt u gebruik maken van NotePad of WordPad. U
     moet het bestand dan opslaan als gewone tekst maar de naam van de file
     heeft NIET ".txt" op het einde!

    Beschikbare opties

     De volgende instellingen kunnen gemaakt worden door deze configuratie
     file. (Commando-regel opties negeren deze instellingen.)

   fullscreen=yes
           Het programma draait in full screen mode, niet in een venster.

   nosound=yes
           Schakelt het geluid uit.

   noquit=yes
           Schakelt de 'stop-knop' uit. (Door op [Escape] drukken of op de
           'venster sluiten knop' te klikken kunt u het programma toch
           afsluiten.)

   noprint=yes
           Schakelt het printen uit.

   printdelay=SECONDEN
           Het printen wordt beperkt tot een print per zoveel seconden.

   printcommand=COMMAND
           (Alleen Linux en Unix)
           Gebruik het COMMAND om een PNG bestand te printen. Is dit niet
           ingesteld dan is het standaard commando:

     pngtopnm | pnmtops | lpr

           De PNG file wordt eerst omgezet naar een NetPBM 'portable anymap',
           vervolgens omgezet naar een PostScript file, welke dan naar de
           printer gestuurd wordt met het "lpr" commando.

   simpleshapes=yes
           Schakelt het draaien van het 'vorm' gereedschap uit. Klikken,
           slepen en loslaten is dan nog nodig om een bepaalde vorm te
           tekenen.

   uppercase=yes
           Alle tekst verschijnt in hoofdletters. Dit is voor kinderen die
           pas hebben leren lezen.

   grab=yes
           Tux Paint zal proberen alle muis en keyboard acties te beperken
           Hiermee worden acties beperkt die de gebruiker buiten Tux Paint
           zouden kunnen brengen ( bijvoorbeeld: met [Alt]-[Tab] door de
           verschillende vensters 'lopen', [Ctrl]-[Escape], enz.)

   nowheelmouse=yes
           Dit schakelt de ondersteuning voor de 'wiel-muis' uit. Normaal
           wandelt u met het wieltje door het selectiemenu rechts.

   saveover=yes
           Dit schakelt het dialoogvenster "Bestand bestaat al
           overschrijven...?" uit. Met deze optie wordt elke oudere versie
           overschreven.

   saveover=new
           In tegenstelling tot de optie hierboven, wordt met deze optie
           steeds weer een nieuw bestand aangemaakt. De oude bestanden worden
           op deze manier nooit overschreven.

   saveover=ask
           (Deze optie is vervallen omdat dit de standaard instelling is.)
           Standaard opent er zich een dialoogvenster met de vraag of het
           oude bestand overschreven mag worden.

   lang=LANGUAGE
           Start Tux Paint op in een van de ondersteunde talen. U kunt uw
           keuze maken uit de volgende talen:

                        +------------------------------------------------------+
                        |english             |Amerikaans-Engels    |           |
                        |--------------------+---------------------+-----------|
                        |britisch-english    |Brits-Engels         |           |
                        |--------------------+---------------------+-----------|
                        |brazilian-portuguese|Portugees-Braziliaans|Braziliaans|
                        |--------------------+---------------------+-----------|
                        |catalan             |Catalaans            |           |
                        |--------------------+---------------------+-----------|
                        |czech               |Tjechisch            |           |
                        |--------------------+---------------------+-----------|
                        |danish              |Deens                |           |
                        |--------------------+---------------------+-----------|
                        |dutch               |Nederlands           |           |
                        |--------------------+---------------------+-----------|
                        |finnish             |Fins                 |           |
                        |--------------------+---------------------+-----------|
                        |french              |Frans                |           |
                        |--------------------+---------------------+-----------|
                        |german              |Duits                |           |
                        |--------------------+---------------------+-----------|
                        |hungarian           |Hongaars             |           |
                        |--------------------+---------------------+-----------|
                        |icelandic           |IJslands             |           |
                        |--------------------+---------------------+-----------|
                        |italian             |italiaans            |           |
                        |--------------------+---------------------+-----------|
                        |norwegian           |Noors                |           |
                        |--------------------+---------------------+-----------|
                        |spanish             |Spaans               |           |
                        |--------------------+---------------------+-----------|
                        |swedish             |Zweeds               |           |
                        |--------------------+---------------------+-----------|
                        |turkish             |Turks                |           |
                        +------------------------------------------------------+

       -----------------------------------------------------------------

    De systeem instellingen teniet doen door middel van de .tuxpaintrc

     Als een van de bovengenoemde opties ingesteld zijn in
     "/etc/tuxpaint/tuxpaint.config", kunt u deze ongedaan maken in uw eigen
     "~/.tuxpaintrc" file.

     Voor true/false opties, zoals "noprint" en "grab", kunt u deze eenvoudig
     gelijk stellen aan 'no' in uw "~/.tuxpaintrc" file:

     noprint=no
     uppercase=no

     Ook kunt u commando-regel opties gebruiken om deze instellingen ongedaan
     te maken. bijvoorbeeld:

     print=yes
     mixedcase=yes

     ----------------------------------------------------------------------

  Commando-regel opties

     Opties kunnen ook op de commando-regel meegegeven worden bij het
     opstarten van Tux Paint. Enkele voorbeelden:

   --fullscreen
   --nosound
   --noquit
   --noprint
   --printdelay=SECONDS
   --simpleshapes
   --uppercase
   --grab
   --nowheelmouse
   --saveover
   --saveovernew
   --lang LANGUAGE
           Deze schakelen de hierboven beschreven opties in.

   --windowed
   --sound
   --quit
   --print
   --printdelay=0
   --complexshapes
   --mixedcase
   --dontgrab
   --wheelmouse
   --saveoverask
           Deze opties kunnen gebruikt worden om instellingen in de
           configuratie files teniet te doen. (Is de optie niet ingesteld in
           het configuratiebestand dan is het teniet doen van de instelling
           natuurlijk onnodig.)

   --locale locale
           Om Tux Paint in een van de ondersteunde talen te starten kijkt u
           het beste in het hoofdstuk "Een andere taal kiezen" Hier kunt u de
           instellingen van de locale strings (bijvoorbeeld, "de_DE@euro"
           voor Duits) vinden.
           (Is uw locale al ingesteld, bijvoorbeeld met "$LANG" omgeving
           (shell) variabele, dan is deze optie onnodig, omdat Tux Paint uw
           omgevingsvariabele herkent.

   --nosysconfig
           Onder Linux en Unix zorgt deze optie ervoor dat de systeem
           configuratie file "/etc/tuxpaint/tuxpaint.conf", niet gelezen
           wordt.
           Alleen uw eigen configuratie bestand, "~/.tuxpaintrc", zal gelezen
           worden (mits deze bestaat).

     ----------------------------------------------------------------------

  Commando-regel informatie opties

     De volgende opties geven informatie op het scherm echter Tux Paint zal
     hiermee niet opstarten.

   --version
           Geeft de versie en de datum van uw Tux Paint programma. Deze optie
           geeft ook aan welke opties ten tijde van het compileren werden
           meegegeven. Voor meer informatie zie (INSTALL.txt en FAQ.txt).

   --copying
           Geeft korte informatie over de licentie waaronder Tux Paint wordt
           uitgebracht.

   --usage
           Geeft een overzicht van alle beschikbare commando-regel opties.

   --help
           Geeft korte help informatie over Tux Paint.

     ----------------------------------------------------------------------

  Een andere taal kiezen.

     Tux Paint is vertaald in een aantal talen. Om een taal te kiezen kunt u
     de code "--lang" als optie meegeven op de commando-regel( bijvoorbeeld:
     "--lang spanish") ook kunt u gebruik maken van "lang=" instelling in de
     configuratie file (bijvoorbeeld: "lang=spanish").

     Tux Paint leest ook de variabelen ingesteld in uw omgevings locale.
     (Deze instelling kunt u teniet doen door op de commando-regel de code
     "--locale" optie mee te geven (zie boven)).

     De volgende talen worden ondersteund:

           +----------------------------------------------------------------+
           |Locale Code|Language           |Language                        |
           |           |(eigen naam)       |(Nederlandse naam)              |
           |-----------+-------------------+--------------------------------|
           |C          |                   |Engels                          |
           |-----------+-------------------+--------------------------------|
           |ca_ES      |Catalan            |Catalaans                       |
           |-----------+-------------------+--------------------------------|
           |cs_CZ      |Cesky              |Tjechisch                       |
           |-----------+-------------------+--------------------------------|
           |da_DK      |Dansk              |Deens                           |
           |-----------+-------------------+--------------------------------|
           |de_DE@euro |Deutsch            |Duits                           |
           |-----------+-------------------+--------------------------------|
           |en_GB      |                   |Brits Engels                    |
           |-----------+-------------------+--------------------------------|
           |es_ES@euro |Espanol            |Spaans                          |
           |-----------+-------------------+--------------------------------|
           |fi_FI@euro |Suomi              |Fins                            |
           |-----------+-------------------+--------------------------------|
           |fr_FR@euro |Franc,ais          |Frans                           |
           |-----------+-------------------+--------------------------------|
           |hu_HU      |Magyar             |Hongaars                        |
           |-----------+-------------------+--------------------------------|
           |is_IS      |Islenska           |IJslands                        |
           |-----------+-------------------+--------------------------------|
           |it_IT@euro |Italiano           |Italiaans                       |
           |-----------+-------------------+--------------------------------|
           |nb_NO      |Norsk (bokmaal)    |Noors (Bokmaal)                 |
           |-----------+-------------------+--------------------------------|
           |nn_NO      |Norsk (nynorsk)    |Noors (Nynorsk)                 |
           |-----------+-------------------+--------------------------------|
           |nl_NL@euro |Nederlands         |Nederlands                      |
           |-----------+-------------------+--------------------------------|
           |pt_BR      |Portuges Brazileiro|Braziliaans Portugees           |
           |-----------+-------------------+--------------------------------|
           |sv_SE@euro |Svenska            |Zweeds                          |
           |-----------+-------------------+--------------------------------|
           |tr_TR@euro |                   |Turks                           |
           +----------------------------------------------------------------+

    De omgevingsvariabele locale instellen

     Het veranderen van uw locale instellingen heeft grote invloed op uw
     systeem!

     Zoals aangegeven hierboven kunt u uw taal kiezen door een optie mee te
     geven op de commando-regel.("--lang" en "--locale"), Tux Paint zal de
     locale setting van uw omgeving overnemen.

     Het volgende geeft uitleg hoe u omgevings locale kunt instellen:

    Linux/Unix gebruikers

     Verzeker u allereerst dat de locale die u wilt gebruiken is
     ingeschakeld. Dit doet u door de file  "/etc/locale.gen" op uw systeem
     te bewerken. Vervolgens voert u het programma  "locale-gen"  als root
     uit.

     NB: Debian gebruikers kunnen het commando "dpkg-reconfigure locales". 
     uitvoeren

     Voordat u Tux Paint start, stelt u uw "$LANG"  omgevingsvariabele in op
     een van de locales zoals hierboven aangegeven. (Wilt u dat alle
     programma's die meertalige ondersteuning bieden vertaald worden dan kunt
     u het volgende in uw login script plaatsen:  ~/.profile, ~/.bashrc,
     ~/.cshrc,  etc.)

     Bijvoorbeeld in de Bourne Shell (BASH):

     export LANG=es_ES@euro ; \
     tuxpaint

     En in de C Shell (TCSH):

     setenv LANG es_ES@euro ; \
     tuxpaint

     ----------------------------------------------------------------------

    Windows gebruikers

     Tux Paint zal de locale instellingen herkennen en de betreffende
     bestanden automatisch gebruiken. Het onderstaande is dan ook alleen maar
     voor mensen die andere talen willen uitproberen.

     Het eenvoudigste is om '--lang' te gebruiken in de snelkoppeling (zie
     voor meer informatie de  "INSTALL.txt"). Echter door een MSDOS Prompt
     venster te gebruiken is het ook mogelijk om een commando als:

     set LANG=es_ES@euro

     te starten. Dit verandert uw instellingen voor de duur van dat DOS
     venster.

     Wilt u echter de zaken permanent veranderen, probeer dan de
     'autoexec.bat' file te bewerken door gebruik te maken van het Windows'
     "sysedit" gereedschap:

    Windows 95/98

    1.   Klik op de 'Start' knop en kies 'uitvoeren...'.

    2.   Type "sysedit" (met of zonder aanhalingstekens).

    3.   Klik 'OK'.

    4.   Zoek het AUTOEXEC.BAT window in de Systeem Configuratie Editor

    5.   En voeg het volgende onderaan het bestand toe:

         set LANG=es_ES@euro

    6.   Sluit de systeem configuratie editor, en beantwoord alle vragen met
         'ja'.

    7.   Herstart uw computer.

     Om de veranderingen op uw hele computer (en voor alle programma's) door
     te voeren is het beter om de regionale instellingen in uw
     configuratiescherm aan te passen:

    1.   Klik op de 'Start' knop en selecteer 'instellingen | configuratie
         scherm  

    2.   Dubbel-klik op "regionale instellingen".

    3.   Selecteer de taal / regio uit het menu.

    4.   Klik 'OK'.

    5.   Start de computer opnieuw op.

     ----------------------------------------------------------------------

Titel scherm

     Als Tux Paint voor het eerst opstart verschijnt er een titelpagina.

     [Title Screenshot]

     Als deze pagina geladen is kunt u op een willekeurig toets drukken of op
     de muis klikken om verder te gaan. (Na ongeveer 30 seconden gaat de
     titelpagina vanzelf weg.)

     ----------------------------------------------------------------------

Hoofd scherm

     Het hoofdscherm is verdeeld in de volgende secties:

             Linkerzijde: Gereedschapbalk

                De gereedschapbalk bevat teken- en bewerkingsgereedschappen.

           [Tools: Paint, Stamp, Lines, Shapes, Text, Magic, Undo, Redo,     
           Eraser, New, Open, Save, Print, Quit]

                     Midden: Tekenpapier

                        Het grootste deel van het scherm is het teken- papier

           [(Canvas)]

                     Rechterzijde: Selectie hulpmiddelen

                        Afhankelijk van het gekozen gereedschap, laat de
                        rechterzijde verschillende dingen zien. Bijvoorbeeld
                        als de kwast geselecteerd is, zijn er aan de
                        rechterzijde verschillende kwasten, naar keuze,
                        beschikbaar. Is het stempel gereedschap geselecteerd
                        dan laat de rechterzijde verschillende beschikbare
                        vormen en / of afbeeldingen zien.

           [Selectors - Brushes, Letters, Shapes, Stamps]

                     Onder: De kleuren

                        A De beschikbare kleuren zijn aan de onderkant te
                        zien.

           [Colors - Black, White, Red, Pink, Orange, Yellow, Green, Cyan,   
           Blue, Purple, Brown, Grey]

                     Onderaan: Help

                        Helemaal aan de onderkant van het scherm verschijnt
                        Tux de Pinguin met handige tips en andere informatie.

           (Bijvoorbeed: 'Kies een vorm. Beweeg de muis om te draaien,      
           klik om te tekenen etc.)

   --------------------------------------------------------------------------

Beschikbare gereedschappen

  Teken gereedschappen

             Kwast

                Met het kwast gereedschap kunt u uit de vrije hand tekenen.
                De kleuren kiest u aan de onderzijde, de kwast kiest aan de
                rechterzijde.
                Houdt u de muistoets ingedrukt dan 'schildert' de kwast, de
                beweging van de muis volgend.
                Terwijl u tekent speelt er een geluidje. Is de kwast groter
                dan klinkt het geluid lager.

   --------------------------------------------------------------------------

                     Stempel gereedschap

                        Het stempel gereedschap is als een stempel uit de
                        stempeldoos of als een sticker die opgeplakt wordt.
                        Hiermee kunt u de beschikbare plaatjes in uw tekening
                        'plakken'.
                        Als u met de muis beweegt dan ziet u de omtrek van
                        het plaatje de muis volgen. Verschillende stempels
                        kennen verschillende geluidseffecten.

   --------------------------------------------------------------------------

   --------------------------------------------------------------------------

                     Lijn gereedschap

                        Met dit gereedschap kunt u rechte lijnen tekenen. U
                        maakt gebruik van de verschillende kwasten en kleuren
                        zoals beschreven bij het kwast-gereedschap.
                        Klik op de muistoets en houd deze toets ingedrukt.
                        Als u de muis beweegt ziet u een hulplijn, deze lijn
                        geeft aan waar de lijn getekend wordt als u de toets
                        loslaat.
                        Bij het loslaten van de muistoets klinkt een
                        "sproing!" geluid.

   --------------------------------------------------------------------------

   --------------------------------------------------------------------------

                     Vorm gereedschap

                        Met dit gereedschap kunt u eenvoudige vormen tekenen.
                        Deze vormen zijn, naar keuze, volledig gevuld met een
                        kleur of bestaan alleen uit lijnen.
                        Selecteer de vorm aan de rechterzijde (cirkel,
                        vierkant, ovaal, etc.).
                        Klik op het papier en sleep met de muis. Sommige
                        vormen veranderen van vorm (rechthoek, ovaal), andere
                        niet (vierkant cirkel).
                        Als u klaar bent laat u de muistoets los.

                             Normale mode

                        Door met de muis te bewegen kunt u nu de vorm
                        draaien.
                        Klikt u opnieuw dan wordt de vorm op het papier
                        gezet.

                             Eenvoudige vormen mode:
                                     Is het programma met de optie
                                     "--simpleshapes"  opgestart dan bestaat
                                     er geen optie om de vorm te draaien.

   --------------------------------------------------------------------------

   --------------------------------------------------------------------------

                     Tekst gereedschap

                        Kies een lettertype aan de rechterzijde en een kleur
                        aan de onderkant. Klik op het papier en typ de
                        gewenste tekst. Terwijl u typt verschijnt de tekst op
                        het scherm.
                        Druk  [Enter]  of de [Return] toets en de cursor gaat
                        een regel naar beneden.
                        Klikt u op een andere plaats in de tekening dan zal
                        uw tekst naar die plaats verschuiven. U kunt dan daar
                        weer verder gaan met typen.

   --------------------------------------------------------------------------

   --------------------------------------------------------------------------

                     Tover gereedschap (Speciale Effecten)

                        Het tover gereedschap is eigenlijk een verzameling
                        gereedschappen. Kies een van de "speciale effecten"
                        aan de rechterzijde. klik en sleep in de tekening om
                        het effect te zien.

                             Regenboog

                        Deze optie is te vergelijken met de kwast. Echter,
                        deze kwast gebruikt alle kleuren van de regenboog.

                             Sparkles

                        Deze optie tekent gele 'sparkles' (spatten) op de
                        tekening.

                             Spiegelen

                        Klikt u in de tekening met deze optie geselecteerd
                        dan zal de tekening gespiegeld worden.

                             Flip

                        Te vergelijken met spiegelen echter nu in verticale
                        richting.

                             Vervagen (blur) van de tekening

                        Deze optie vervaagt de tekening.

                             Blokken

                        Deze optie zorgt ervoor dat de tekening er
                        'blokkerig' gaat uitzien.

                             Negatief

                        Met deze optie worden de kleuren geinverteerd, zwart
                        wordt wit en omgekeerd.

                             Vervagen van kleuren (Fade)

                        Hiermee vervaagt u de kleuren. Gaat u met de muis een
                        aantal keren over dezelfde plaats dan zal deze plek
                        uiteindelijk wit worden.

                             Krijt optie

                        Deze optie zorgt ervoor dat de tekening er als een
                        krijttekening gaat uitzien.

                             Druip-effect

                        Hiermee laat u de verf van de tekening druipen.

                             Aandikken

                        Zorgt ervoor dat de donkere kleuren aangedikt (nog
                        dikker) worden.

                             Verdunnen

                        Vergelijkbaar met 'aandikken' echter omgekeerd.

                             Vullen

                        Hiermee vult u een deel van de tekening met een kleur
                        naar keuze.

               -------------------------------------------------

                             Gum gereedschap

                        Dit gereedschap is te vergelijken met de kwast.
                        Alleen schildert de gum alleen met wit. De tekening
                        wordt als het ware uitgegumd.
                        Als u met de muis beweegt ziet u een groot vierkant
                        de muis volgen. Het gedeelte dat door dit vierkant
                        begrensd is zal wit worden.
                        Onder het gummen zal er een geluidje klinken.

     ----------------------------------------------------------------------

   --------------------------------------------------------------------------

Andere gereedschappen

             Ongedaan maken (undo)

                Klikt u op dit gereedschap dan zal de laatste actie ongedaan
                gemaakt worden. U kunt meerdere stappen achter elkaar
                ongedaan maken.
                NB: U kunt hiervoor de gebruikelijke sneltoets [Control]-[Z]
                gebruiken.

   --------------------------------------------------------------------------

                     Opnieuw doen (redo)

                        Met dit gereedschap maakt u het ongedaan maken
                        ongedaan. Met andere woorden u kunt het ongedaan
                        maken weer herstellen.
                        Deze toets kunt u zo vaak gebruiken als u de ongedaan
                        maken toets heeft gebruikt."
                        NB: De hierbij behorende sneltoets is [Control]-[R]

   --------------------------------------------------------------------------

                     Nieuw

                        Door op deze toets te drukken begint u een nieuwe
                        tekening. Het programma vraagt u eerst om een
                        bevestiging.
                        NB: De sneltoets hierbij is [Control]-[N].

   --------------------------------------------------------------------------

                     Open

                        Deze optie laat u een overzicht zien van alle
                        tekeningen die u tot dusver heeft bewaard. Zijn er
                        meer tekeningen dan ruimte op het scherm beschikbaar
                        is dan kunt u eenvoudig de "Omhoog" en de "Omlaag"
                        pijltoetsen gebruiken.

                        Klik op de tekening om te selecteren..

     *   Klik op de groene "Open" knop om de tekening te laden.

         (U kunt ook dubbel-klikken op de tekening zelf.)

     *   Klik op de bruine "wissen" (prullenbak) knop om de geselecteerde
         tekening te verwijderen. Vervolgens wordt u om een bevestiging
         gevraagd.)

     *   Of u klikt op de rode "terug" knop om terug te keren naar de
         tekening.

                Kiest u ervoor een bestaande tekening te openen en u heeft uw
                huidige tekening nog niet opgeslagen dan zult u gevraagd
                worden of u deze wilt bewaren of niet. (Zie ook "Bewaren".)
                NB: U kunt ook de sneltoets [Control]-[O] gebruiken om een
                nieuw bestand te openen.

   --------------------------------------------------------------------------

                     Bewaren

                        Hiermee bewaart u uw huidige tekening.
                        Is de tekening nog niet eerder bewaard dan zal er een
                        nieuwe 'tumbnail' in uw lijst gemaakt worden (er
                        wordt dus een nieuw bestand aangemaakt worden)
                        NB: Het programma vraagt niet om een bestandsnaam. De
                        tekening zal worden bewaart en u hoort een
                        fototoestel geluid.
                        Echter als u de tekening al eerder heeft bewaard dan
                        zal het programma u eerst vragen of u de oude
                        tekening wilt overschrijven of dat u een nieuw
                        bestand wilt aanmaken.

           (NB: Heeft u de opties "saveover" of "saveovernew" ingesteld dan
           zal het programma u niets vragen. Zie voor meer informatie het
           "Opties" hoofdstuk
           NB: De sneltoets voor een tekening op te slaan is zoals
           gebruikelijk [Control]-[S] .

   --------------------------------------------------------------------------

                     Afdrukken (Print)

                        Klik op deze knop en uw tekening zal afgedrukt
                        worden.

                             Afdrukken uitschakelen

                        Heeft u de "noprint" optie ingesteld (met
                        "noprint=yes" in de Tux Paint configuratie file) of u
                        heeft bij het opstarten de optie "--noprint"
                        meegegeven dan zal de "Afdrukken / Print" knop niet
                        werken.
                        Zie hiervoor ook het "Opties" hoofdstuk van dit
                        document.

                             Het aantal afdrukken begrenzen

                        Heeft u de "printdelay" optie ingesteld in de
                        configuratie file "printdelay=SECONDEN" of door bij
                        het opstarten de optie "--printdelay=SECONDEN" mee te
                        starten. Dan kan er slechts elke SECONDEN een afdruk
                        gemaakt worden.
                        Bijvoorbeeld met de "printdelay=60" kunt u slechts
                        elke minuut een afdruk maken.
                        Voor meer informatie zie het hoofdstuk "Opties".

                             Andere Print Opties

                        (Alleen Linux en Unix)
                        Het afdrukken onder Linux gebeurt door het PNG
                        bestand om te zetten naar een Postscript bestand. Dit
                        PostScript bestand wordt vervolgens naar de printer
                        gestuurd.

     pngtopnm | pnmtops |lpr

                Dit commando kan verandert worden door de "printcommand"
                waarde in Tux Paint's configuratie bestand te wijzigen.
                Voor meer informatie ziet u weer het "Opties" hoofdstuk.

   --------------------------------------------------------------------------

                     Stoppen

                        U kunt Tux Paint op een aantal manieren afsluiten,
                        door op de "Stop" knop te klikken, door het Tux Paint
                        venster te sluiten of door op de "Escape" toets te
                        drukken.
                        (NB: De "Stop" knop kan uitgeschakeld
                        zijn!(bijvoorbeeld door bij het opstarten de
                        "--noquit" optie mee te geven) Echter de [Escape]
                        toets blijft werken. Voor meer informatie zie het
                        "Opties" hoofdstuk.
                        Voordat het programma afsluit zal het u eerst om een
                        bevestiging vragen.
                        Als u ervoor kiest om het programma af te sluiten en
                        u heeft uw huidige werk nog niet opgeslagen dan zal
                        het programma u eerst vragen om uw tekening op te
                        slaan. Is het geen nieuwe tekening dan vraagt het
                        programma eerst nog een bevestiging om uw oude werk
                        te overschrijven of het werk als een nieuwe tekeing
                        op te slaan.(Zie ook het hoofdstuk "Opslaan". )
                        NB: De laatst opgeslagen tekening zal automatisch
                        geladen worden bij het opstarten van Tux Paint!

   --------------------------------------------------------------------------

Andere afbeeldingen in Tux Paint laden

     Het dialoogvenster van Tux Paint laat u alleen met Tux Paint programma
     gemaakte tekeningen zien. Toch is het mogelijk om ander afbeeldingen of
     foto's in te laden.

     Hiervoor is het nodig om de afbeeldingen naar het PNG (Portable Network
     Graphic) formaat om te zetten. Daarna plaatst u de afbeelding in de
     werkdirectory van Tux Paint. ("~/.tuxpaint/saved/" onder Linux en Unix,
     "userdata\saved\" onder Windows.)

De 'tuxpaint-import' gebruiken

     Linux en Unix gebruikers kunnen de "tuxpaint-import" shell-script
     toepassen. Deze wordt meegeinstalleerd bij de installatie van Tux Paint.
     Het maakt gebruik van de NetPBM gereedschappen om de afbeelding om te
     zetten.("anytopnm"), Het formaat van de afbeelding zal worden aangepast
     zodat het binnen het Tux Paint's venster past ("pnmscale"), vervolgens
     wordt de afbeelding omgezet naar een PNG formaat ("pnmtopng").

     Tux Paint maakt ook gebruik van het "date" commando om de huidige datum
     en tijd te lezen. Dit is immers hoe Tux Paint namen geeft aan de
     gemaakte tekeningen. (Tux Paint vraagt geen bestandsnamen wanneer u een
     bestand wilt openen, weet u nog?)

     Om de 'tuxpaint-import' te gebruiken typt u het eenvoudig op he
     commando-regel en geeft u de om-te-zetten bestandsnaam op.

     De afbeeldingen worden omgezet en in de betreffende Tux Paint map
     geplaatst. NB: Doet u dit voor bijvoorbeeld u zoon of dochter dan moet u
     het commando uitvoeren onder hun inlognaam.

     Een voorbeeld:

     $ tuxpaint-import grandma.jpg
     grandma.jpg -> /home/username/.tuxpaint/saved/20020921123456.png
     jpegtopnm: WRITING A PPM FILE

     De eerste regel ("tuxpaint-import grandma.jpg") is het uit te voeren
     commando. De volgende twee regels zijn output van het programma.

     Start u vervolgens Tux Paint dan zult u zien dat de betreffende
     afbeelding beschikbaar is bij de 'open' optie. U hoeft alleen nog maar
     te dubbel-klikken op de 'thumbnail'.

Met de hand omzetten....

     Helaas moeten Windows gebruikers de afbeeldingen met de hand omzetten.

     Hiervoor moet u gebruik maken van een programma dat in staat is een
     afbeelding op te slaan in het PNG formaat. (Zie hiervoor het "PNG.txt"
     bestand.)

     De grootte van de afbeelding dient beperkt te zijn tot 448 pixels bij
     376 pixels.

     Sla de afbeelding op in het PNG formaat waarbij u de afbeelding de voor
     Tux Paint gebruikelijke naam geeft:

     YYYYMMDDhhmmss.png

     *   YYYY = Jaar (2003)

     *   MM = Maand (01-12)

     *   DD = Dag (01-31)

     *   HH = Uur, in 24-uurs formaat (00-23)

     *   mm = Minuten (00-59)

     *   ss = Seconden (00-59)

     bijvoorbeeld:

     20020921130500

     - voor September 21, 2002, 13:05:00

     De afbeelding dient u te plaatsen in de betreffende Tux Paint directory.
     (Zie boven.)

     Voor Windows is dit de "userdata" map.

     ----------------------------------------------------------------------

                              Tux Paint uitbreiden

     Tux Paint is eenvoudig uit te breiden. Dit kunt u doen door bestanden
     (zoals kwasten en / of stempels) op w harde schijf te plaatsen.

     NB: Om Tux Paint bekend te maken met de veranderingen moet u het
     programma opnieuw opstarten.

Waar moeten deze bestanden komen?

  Standaard files:

     Tux Paint zoekt naar de verschillende data-bestanden in de 'data'
     directory.

    Linux en Unix

     Onder dit besturingssysteem is een en ander afhankelijk van de
     ingestelde waarde bij de compilatie van Tux Paint. "DATA_PREFIX" Voor
     meer informatie bekijkt u het bestand INSTALL.txt.

     Standaard is echter de volgende directory in gebruik:

     /usr/local/share/tuxpaint/

     Heeft u het programma als een RPM pakket verkregen dan is waarschijnlijk
     de volgende map in gebruik:

     /usr/share/tuxpaint/

    Windows

     Tux Paint zoekt naar een directory genaamd 'data'. Dit is de map die het
     installatieprogramma gebruikte om Tux Paint te installeren:

     C:\Program Files\TuxPaint\data

     ----------------------------------------------------------------------

  Persoonlijke bestanden

     Kwasten, stempels, lettertypes etc. kunnen ook in uw persoonlijke map
     geplaatst worden.

    Linux en Unix

     Uw persoonlijke Tux Paint directory is "~/.tuxpaint/".

     Bijvoorbeeld uw home directory is "/home/karl", dan is uw Tux Paint
     directory "/home/karl/.tuxpaint/".

     Vergeet u de punt (".") niet voor de 'tuxpaint'!

    Windows

     Uw persoonlijke Tux Paint map is hier "userdata" genoemd en bevindt zich
     in dezelfde map als de executable file:

     C:\Program Files\TuxPaint\userdata

     Om kwasten, stempels en lettertypes toe te voegen kunt u het beste
     subdirectories aanmaken onder uw persoonlijke Tux Paint directory met de
     namen "kwasten", "stempels" en "fonts".

     Bijvoorbeeld u heeft een kwast gemaakt met de naam "bloem.png", deze
     plaatst u dan in "~/.tuxpaint/kwasten/" onder Linux of Unix.)

     ----------------------------------------------------------------------

Kwasten

     De kwasten en lijnen die u in Tux Paint gebruikt zijn gewoon zwart-wit
     PNG afbeeldingen.

     De transparantie van de PNG afbeelding wordt gebruikt voor de vorm van
     de kwast. Dit betekent dat de vorm 'anti-alias' kan zijn en gedeeltelijk
     transparant.

     Afbeeldingen van kwasten mogen niet groter zijn dan 40 pixels bij
     40 pixels.

     U plaatst ze in de "brushes" directory.

     NB: Zien uw kwasten er allemaal uit als vierkanten of rechthoeken dan
     heeft u vergeten de alpha transparantie toe te passen! Zie de
     documentatie in de file "PNG.txt"!

     ----------------------------------------------------------------------

Stempels

     Alle stempel-files worden in de "stempels" directory geplaatst. Het is
     handig om de diverse stempels op hun beurt weer over meerdere submappen
     te verdelen. U heeft bijvoorbeeld sub-directories voor stempels
     betreffende "halloween" en "kerstmis"

  Afbeeldingen

     Rubber Stempels in Tux Paint kunnen bestaan uit een aantal files. Het
     bestand dat nodig is, is natuurlijk de afbeelding zelf.

     De stempels, zoals ze door Tux Paint gebruikt worden, zijn PNG
     afbeeldingen. Ze kunnen zwart-wit of in kleur zijn. De alpha
     transparantie van de PNG afbeelding bepaalt de eigenlijke vorm. Anders
     zouden al u stempels rechthoeken zijn.

     Deze PNG's kunnen elke afmeting hebben echter in de praktijk lijkt
     100 pixels bij 100 pixels (100 x 100) groot genoeg te zijn voor
     Tux Paint.

     NB: Zien al uw stempels er uit als rechthoeken of vierkanten, dan is dat
     omdat u vergeten bent alpha transparantie te gebruiken! Zie voor meer
     informatie het bestand "PNG.txt".

     ----------------------------------------------------------------------

  Informatieve tekst

     Een tekst (".TXT") file met dezelfde naam als het PNG bestand.
     (bijvoorbeeld "picture.png" met de tekst file "picture.txt" opgeslagen
     in dezelfde directory.)

    Meertalige ondersteuning

     Regels beginnend met "xx=" (waar "xx" staat voor een van de ondersteunde
     talen bijvoorbeeld "de" voor Duits, "fr" voor Frans, enz.) zullen
     gebruikt worden onder de verschillende ondersteunde locales.

     Is er geen vertaling beschikbaar voor de gebruikte locale, de default
     string (de eerste regel, welke in het Engels is) wordt dan gebruikt.

    Windows Gebruikers

     Gebruikt u NotePad of WordPad om deze files te bewerken of aan te maken,
     Zorgt u dan ervoor deze files op te slaan als gewone tekst files met de
     extensie ".txt" op het einde van de bestandsnaam...

     ----------------------------------------------------------------------

  Geluids effecten

     WAVE (".WAV") files met dezelfde naam als de PNG file in dezelfde
     directory. (bijvoorbeeld "picture.png" met het geluids effect
     "picture.wav".)

    Taal ondersteuning

     Geluiden voor de verschillende locales (het geluid is bijvoorbeeld een
     woord of naam en u wilt de vertaalde versie van het betreffende woord),
     hiervoor maakt u WAV files aan met de naam van de locale in de filenaam:
     "STAMP_LOCALE.wav"

     Het geluidseffect behorend bij "picture.png", als Tux Paint gestart
     wordt in het Spaans, is dan "picture_es.wav". In het Frans:
     "picture_fr.wav".

     Kan er geen aangepast geluidseffect geladen worden dan zal Tux Paint
     proberen om het standaard geluid te gebruiken. (bijvoorbeeld:
     "picture.wav")

     ----------------------------------------------------------------------

  Stempel opties

     Behalve een vorm, een beschrijving en een geluid kunt u een stempel nog
     andere attributen meegeven. Dit doet u door een 'data file' aan te maken
     voor de stempel.

     Een stempel data file is een tekst file welke de opties bevat.

     Deze file heeft dezelfde naam als de PNG afbeelding maar een ".dat"
     extensie. (Bijvoorbeeld de "picture.png"'s data file is de tekst file
     "picture.dat" in dezelfde directory.)

    Gekleurde stempels

     Stempels kunnen een "kleur" of een "tint" hebben.

      Stempels met een kleur

     "Gekleurde" stempels werken als de kwasten - u kiest de stempel voor de
     vorm , vervolgens kiest u de kleur. (Symbool stempels, zoals de
     rekenkundige en de muziek stempels zijn hier een voorbeeld van.)

     Van de originele afbeelding wordt alleen de transparantie ("alpha"
     kanaal) gebruikt. De kleur van de stempel is gevuld.

     Voeg het woord "colorable" toe aan de data file van de stempel.

      Stempels met een tint

     "Getinte" stempels zijn vergelijkbaar met de "gekleurde stempels",
     echter bij de "getinte" stempels zijn de details van de originele
     afbeelding bewaard. (Met andere woorden, de originele afbeelding wordt
     gebruikt echter de kleurverzadiging wordt veranderd aan de hand van de
     gekozen kleur.)

     Voeg het woord "tintable" toe aan de data file van de stempel.

    Windows gebruikers

     U kunt NotePad of WordPad gebruiken om deze files aan te maken. Opslaan
     als tekst file met de ".dat" extensie op het einde.

     ----------------------------------------------------------------------

Lettertypes / fonts

     De door Tux Paint gebruikte fonts zijn TrueType Fonts (TTF).

     Deze kunt u eenvoudig in de "fonts" directory plaatsen. Tux Paint zal
     bij het opstarten het lettertype laden. De fonts zijn dan beschikbaar in
     vier groottes.

     ----------------------------------------------------------------------

                                Meer Informatie

     Voor meer informatie leest u eerst de documentatie die geleverd wordt
     bij Tux Paint.

     Heeft u hulp nodig, neemt u dan contact op met New Breed Software:

     http://www.newbreedsoftware.com/

     Ook u kunt deelnemen in de diversen Tux Paint mailing lists:

     http://www.newbreedsoftware.com/tuxpaint/lists/
