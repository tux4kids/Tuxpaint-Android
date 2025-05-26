/* tp_magic_example.c

   Ett exempel på ett "Magic"-verktygsplugin för Rita med Tux
   maj 10, 2024
*/


/* Inkludering av huvudfiler */
/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>             // För "strdup()"
#include <libintl.h>            // För "gettext()"

#include "tp_magic_api.h"       // Rita med Tux "Magic" verktyg API-huvud
#include "SDL_image.h"          // För IMG_Load(), för att ladda vår PNG-ikon
#include "SDL_mixer.h"          // För Mix_LoadWAV(), för att ladda våra ljudeffekter


/* Verktygsuppräkningar: */
/* ---------------------------------------------------------------------- */

/* What tools we contain: */


enum
{
  VERKTYG_ONE,                  // Blir '0'
  VERKTYG_TWO,                  // Blir '1'
  NUM_TOOLS                     // Blir '2'
};


/* Listor med filnamn för ljud och ikoner som ska laddas vid start: */

const char *ljud_filnamn[NUM_TOOLS] = {
  "verktyg_ett.wav",
  "verktyg_två.wav"
};

const char *ikon_filnamn[NUM_TOOLS] = {
  "verktyg_ett.png",
  "verktyg_två.png"
};


/*
OBS: Vi använder ett makro som heter "gettext_noop()" nedan i vissa
matriser med strängar (char *) som innehåller namn och beskrivningar av
våra "Magic"-verktyg.  Detta gör att strängarna kan lokaliseras till
andra språk.
*/


/* En lista med namn för verktygen */

const char *verktygsnamn[NUM_TOOLS] = {
  gettext_noop("Ett verktyg"),
  gettext_noop("Ett annat verktyg")
};


/* Hur man grupperar verktygen med andra liknande verktyg, inom "Magic"-väljaren: */

const int verktyg_grupper[NUM_TOOLS] = {
  MAGIC_TYPE_PAINTING,
  MAGIC_TYPE_DISTORTS
};


/* En lista med beskrivningar av verktygen */

const char *verktyg_beskrivningar[NUM_TOOLS] = {
  gettext_noop("Detta är exempel på verktygsnummer 1."),
  gettext_noop("Detta är exempel på verktygsnummer 2.")
};



/* Våra globala variabler: */
/* ---------------------------------------------------------------------- */

/* Ljudeffekter: */
Mix_Chunk *ljud_effekter[NUM_TOOLS];

/* Den aktuella färgen (ett "RGB"-värde (röd, grön, blå) som användaren
har valt i Rita med Tux (för verktyg 1): */
Uint8 example_r, example_g, example_b;

/* Den storlek som användaren har valt i Rita med Tux (för verktyg 2): */
Uint8 example_storlek;


/* Våra lokala funktionsprototyper: */
/* ---------------------------------------------------------------------- */

/*
Dessa funktioner anropas av andra funktioner inom vårt plugin, så vi
tillhandahåller en "prototyp" av dem, så att kompilatorn vet vad de
accepterar och returnerar.  Detta gör att vi kan använda dem i andra
funktioner som deklareras _före_ dem.
*/

void example_drag(magic_api * api, int som, SDL_Surface * malarduk,
                  SDL_Surface * ogonblicksbild, int gammal_x, int gammal_y,
                  int x, int y, SDL_Rect * uppdatering_rect);

void example_line_callback(void *pekare, int som, SDL_Surface * malarduk,
                           SDL_Surface * ogonblicksbild, int x, int y);


/* Inställningsfunktioner: */
/* ---------------------------------------------------------------------- */

/*
Kontroll av API-version

Den löpande kopian av Rita med Tux som har laddat oss frågar oss först
vilken version av Rita med Tux 'Magic' tool plugin API vi byggdes mot.  Om
det anser att vi är kompatibla kommer vi att användas!

Allt vi behöver göra här är att returnera "TP_MAGIC_API_VERSION", som
definieras (#define) i headerfilen "tp_magic_api.h".
*/

Uint32 example_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}


/*
Initiering av Samhain

Detta händer en gång, när Rita med Tux startar och laddar alla plugins
för "Magic"-verktyget.  (Förutsatt att det vi fick tillbaka från
api_version var acceptabelt!)

Allt vi gör i det här exemplet är att ladda våra ljudeffekter, som vi
kommer att använda senare (i example_click(), example_drag() och
example_release()) när användaren använder våra Magic-verktyg.

Det minne vi allokerar här för att lagra ljuden kommer att frigöras (aka
released, aka deallocated) när användaren avslutar Rita med Tux, när
vår example_shutdown()-funktion anropas.
*/

int example_init(magic_api *api, Uint8 inaktiverade_funktioner,
                 Uint8 komplexitet_niva)
{
  int i;
  char filnamn[1024];

  for (i = 0; i < NUM_TOOLS; i++)
  {
    /*
       Sätt ihop filnamnet från "ljud_filnamn[]"-matrisen till en fullständig
       sökväg till en riktig fil.

       Använd "api->data_directory" för att ta reda på var våra ljud ska vara.
       (Kommandot "tp-magic-config --dataprefix" skulle ha berättat för oss när
       vi installerade vårt plugin och dess data)
     */
    snprintf(filnamn, sizeof(filnamn), "%ssounds/magic/%s",
             api->data_directory, ljud_filnamn[i]);

    printf("Försöker ladda %s ljudfil\n", filnamn);

    /* Försök att ladda filen! */

    ljud_effekter[i] = Mix_LoadWAV(filnamn);
  }

  return (1);
}


/*
Rapportera vårt verktygsantal

Rita med Tux behöver veta hur många "Magic"-verktyg vi kommer att
tillhandahålla. Returnera det numret här.  (Vi tar helt enkelt värdet
på 'NUM_TOOLS' från vår 'enum' ovan!)

När Rita med Tux startar upp och laddar insticksprogram anropar den några
av följande installationsfunktioner en gång för varje verktyg vi
rapporterar.
*/
int example_get_tool_count(magic_api *api)
{
  return (NUM_TOOLS);
}


/*
Ladda våra ikoner

När Rita med Tux startar upp och laddar in plugins ber den oss att
tillhandahålla ikoner för verktygsknapparna "Magic".
*/
SDL_Surface *example_get_icon(magic_api *api, int som)
{
  char filnamn[1024];

  /*
     Sätt ihop filnamnet från "ikon_filnamn[]"-matrisen till en fullständig
     sökväg till en riktig fil.

     Använd "api->data_directory" för att räkna ut var våra ljud ska vara.
     (Kommandot "tp-magic-config --dataprefix" skulle ha berättat för oss när
     vi installerade vårt plugin och dess data)

     Vi använder "som" (vilket av våra verktyg som Rita med Tux frågar om)
     som ett index i matrisen.
   */
  snprintf(filnamn, sizeof(filnamn), "%simages/magic/%s",
           api->data_directory, ikon_filnamn[som]);

  printf("Försöker ladda %s-ikonen\n", filnamn);

  /* Försök att ladda bilden och returnera resultatet till Rita med Tux: */

  return (IMG_Load(filnamn));
}


/*
Rapportera våra namn på "Magic"-verktyg

När Rita med Tux startar upp och laddar in plugins ber den oss att ange
namn (etiketter) för verktygsknapparna "Magic".
*/
char *example_get_name(magic_api *api, int som)
{
  const char *our_name_english;
  const char *our_name_localized;

  /*
     Hämta vårt namn från matrisen "verktygsnamn[]".

     Vi använder 'som' (vilket av våra verktyg som Rita med Tux frågar om)
     som ett index i matrisen.
   */
  our_name_english = verktygsnamn[som];


  /*
     Returnera en lokaliserad (även kallad översatt) version av vårt namn, om
     möjligt.

     Vi skickar "gettext()" den engelska versionen av namnet från vår array.
   */
  our_name_localized = gettext(our_name_english);


  /*
     Slutligen duplicerar du strängen till en ny del av minnet och skickar den
     till Rita med Tux.  (Rita med Tux håller reda på strängen och frigör
     den åt oss, så att vi får en sak mindre att hålla reda på)
   */
  return (strdup(our_name_localized));
}


/*
Rapportera våra "magiska" verktygsgrupper

När Rita med Tux startar upp och laddar in plugins ber den oss att ange
var verktyget ska grupperas.
*/
int example_get_group(magic_api *api, int som)
{
  /*
     Returnera vår grupp, som finns i matrisen "verktyg_grupper[]".

     Vi använder 'som' (vilket av våra verktyg som Rita med Tux frågar om)
     som ett index i matrisen.
   */
  return (verktyg_grupper[som]);
}


/*
Returnera grupperings-/beställningsnummer

När Rita med Tux startar upp och laddar insticksprogram ber den oss att
ange ett numeriskt värde som används för att sortera "Magic"-verktyg
inom en grupp.  Verktygen sorteras utifrån detta nummer, och de som har
samma nummer sorteras i alfabetisk ordning efter sitt lokaliserade namn (se
'example_get_name').
*/
int *example_get_order(int som)
{
  return 0;
}


/*
Rapportera våra beskrivningar av "Magic"-verktyget

När Rita med Tux startar upp och laddar in plugins ber den oss att ge
beskrivningar av varje "Magic"-verktyg.
*/
char *example_get_description(magic_api *api, int som, int lage)
{
  const char *var_desc_engelska;
  const char *var_beskrivning_lokaliserad;

  /*
     Hämta vår beskrivning från matrisen "verktyg_beskrivningar[]".

     Vi använder 'som' (vilket av våra verktyg som Rita med Tux frågar om)
     som ett index i matrisen.
   */
  var_desc_engelska = verktyg_beskrivningar[som];


  /*
     Returnera en lokaliserad (även kallad översatt) version av vår
     beskrivning, om möjligt.

     Vi skickar "gettext" den engelska versionen av beskrivningen från vår
     matris.
   */
  var_beskrivning_lokaliserad = gettext(var_desc_engelska);


  /*
     Slutligen duplicerar du strängen till en ny del av minnet och skickar den
     till Rita med Tux.  (Rita med Tux håller reda på strängen och frigör
     den åt oss, så att vi får en sak mindre att hålla reda på)
   */

  return (strdup(var_beskrivning_lokaliserad));
}


// Rapportera om vi accepterar färger

int example_requires_colors(magic_api *api, int som)
{
  if (som == VERKTYG_ONE)
    return 1;
  else
    return 0;
}


// Rapportera vilka lägen vi arbetar i

int example_modes(magic_api *api, int som)
{
  /*
     Båda våra verktyg är målade (inget av dem påverkar helskärmen), så
     vi returnerar alltid 'MODE_PAINT'
   */

  return MODE_PAINT;
}


// Rapportera om verktygen erbjuder storleksalternativ

Uint8 example_accepted_sizes(magic_api *api, int som, int lage)
{
  if (som == VERKTYG_ONE)
    return 1;
  else
    return 4;
}


// Återgå till vårt standardstorleksalternativ

Uint8 example_default_size(magic_api *api, int som, int lage)
{
  return 1;
}


/*
Stäng av

Rita med Tux håller på att avslutas.  När programmet avslutas ber det
alla plugins att "städa upp" efter sig själva.  Vi laddade till exempel
några ljudeffekter vid start (i vår funktion example_init()), så vi bör
frigöra det minne som används av dem nu.
*/
void example_shutdown(magic_api *api)
{
  int i;

  /*
     Frigör (aka release, aka deallocate) minnet som används för att lagra
     ljudeffekterna som vi laddade in under example_init():
   */
  for (i = 0; i < NUM_TOOLS; i++)
    Mix_FreeChunk(ljud_effekter[i]);
}


/* Funktioner som reagerar på händelser i Rita med Tux: */
/* ---------------------------------------------------------------------- */

/* Påverkar duken vid klick: */

void
example_click(magic_api *api, int som, int lage,
              SDL_Surface *malarduk, SDL_Surface *ogonblicksbild, int x,
              int y, SDL_Rect *uppdatering_rect)
{
  /*
     I vårt fall är ett enda klick (som också är början på en dragning!)
     identiskt med vad dragning gör, men bara på en punkt i stället för
     över en linje.

     Därför "fuskar" vi här genom att anropa vår funktion "example_draw()"
     med (x,y) för både start- och slutpunkterna för en linje.
   */

  example_drag(api, som, malarduk, ogonblicksbild, x, y, x, y,
               uppdatering_rect);
}


/* Påverkar duken vid dragning: */
void
example_drag(magic_api *api, int som,
             SDL_Surface *malarduk, SDL_Surface *ogonblicksbild,
             int gammal_x, int gammal_y, int x, int y,
             SDL_Rect *uppdatering_rect)
{
  /*
     Anropa Rita med Tuxs "line()"-funktion (linjeövergång).

     Den kommer att beräkna en rak linje mellan (gammal_x,gammal_y) och
     (x,y). För varje N steg längs linjen (i det här fallet är N '1')
     anropar den _vår_ funktion, "example_line_callback()", och skickar de
     aktuella X,Y-koordinaterna längs linjen, samt andra användbara saker
     (vilket av våra "Magic"-verktyg som används och de aktuella och
     ögonblicksbildsdukarna).
   */
  SDL_LockSurface(ogonblicksbild);
  SDL_LockSurface(malarduk);

  api->line((void *) api, som, malarduk, ogonblicksbild,
            gammal_x, gammal_y, x, y, 1, example_line_callback);

  SDL_UnlockSurface(malarduk);
  SDL_UnlockSurface(ogonblicksbild);

  /*
     Om det behövs kan du byta ut X- och/eller Y-värdena så att
     koordinaterna (gammal_x,gammal_y) alltid är längst upp till vänster
     och koordinaterna (x,y) alltid är längst ned till höger, så att de
     värden vi anger i "uppdatering_rect" blir meningsfulla:
   */

  if (gammal_x > x)
  {
    int temp = gammal_x;

    gammal_x = x;
    x = temp;
  }
  if (gammal_y > y)
  {
    int temp = gammal_y;

    gammal_y = y;
    y = temp;
  }


  /*
     Fyll i elementen i "uppdatering_rect" SDL_Rect-strukturen som Rita med
     Tux delar med oss, och talar därför om för Rita med Tux vilken del av
     duken som har ändrats och bör uppdateras.
   */

  if (som == VERKTYG_ONE)
  {
    uppdatering_rect->x = gammal_x;
    uppdatering_rect->y = gammal_y;
    uppdatering_rect->w = (x - gammal_x) + 1;
    uppdatering_rect->h = (y - gammal_y) + 1;
  }
  else
  {
    uppdatering_rect->x = gammal_x - example_storlek;
    uppdatering_rect->y = gammal_y - example_storlek;
    uppdatering_rect->w = (x + example_storlek) - uppdatering_rect->x + 1;
    uppdatering_rect->h = (y + example_storlek) - uppdatering_rect->y + 1;
  }

  /*
     Spela upp lämplig ljudeffekt

     Vi beräknar ett värde mellan 0-255 för var musen befinner sig
     horisontellt över duken (0 är vänster, ~128 är mitten, 255 är
     höger).

     Det här är de exakta värden som Rita med Tuxs "playsound()" vill ha
     för att avgöra vilken högtalare ljudet ska spelas upp i. (Så ljudet
     kommer att panorera från högtalare till högtalare när du drar musen
     runt på duken!)
   */
  api->playsound(ljud_effekter[som], (x * 255) / malarduk->w,   /* Vänster/höger panorering */
                 255 /* Nära/långt avstånd (loudness) */ );
}


/* Påverka duken när den släpps: */

void
example_release(magic_api *api, int som,
                SDL_Surface *malarduk, SDL_Surface *ogonblicksbild, int x,
                int y, SDL_Rect *uppdatering_rect)
{
  /*
     Ingen av våra effekter gör något speciellt när musen släpps från
     ett klick eller en klick-och-drag, så det finns ingen kod här...
   */
}


/*
Acceptera färger

När något av våra "Magic"-verktyg aktiveras av användaren, om verktyget
accepterar färger, skickas det aktuella färgvalet till oss.

Om något av våra färgaccepterande verktyg är aktivt när användaren
ändrar sitt val kommer vi dessutom att informeras om det.

Färgen anges som RGB-värden (rött, grönt och blått) från 0 (mörkast)
till 255 (ljusast).
*/
void example_set_color(magic_api *api, int which, SDL_Surface *malarduk,
                       SDL_Surface *ogonblicksbild, Uint8 r, Uint8 g, Uint8 b,
                       SDL_Rect *uppdatering_rect)
{
  /*
     Vi lagrar helt enkelt RGB-värdena i de globala variabler som vi
     deklarerade högst upp i den här filen.
   */

  example_r = r;
  example_g = g;
  example_b = b;
}


/*
Acceptera storlekar

När något av våra "Magic"-verktyg aktiveras av användaren, om verktyget
erbjuder storlekar, skickas det aktuella storleksvalet till oss.

Om användaren ändrar verktygets storlek får vi dessutom information om
detta.

Storleken kommer in som ett osignerat heltal (Uint8) mellan 1 och det
värde som returneras av vår example_accepted_sizes()-funktion under
installationen.
*/
void example_set_size(magic_api *api, int which, int mode,
                      SDL_Surface *malarduk, SDL_Surface *ogonblicksbild,
                      Uint8 storlek, SDL_Rect *uppdatering_rect)
{
  /*
     Spara den nya storleken i den globala variabeln som vi deklarerade
     högst upp i den här filen.
   */

  example_storlek = storlek * 4;
}


/* Den magiska effekten Rutiner! */
/* ---------------------------------------------------------------------- */

/*
Vår "callback"-funktion

Vi gör "arbetet" i denna återuppringning.  Vår plugin-fil har bara en.
Vissa plugins för "magiska" verktyg kan ha fler, beroende på vilka
verktyg de tillhandahåller.  Vissa har inga (eftersom de inte är
klick-och-drag-verktyg i målningsstil).

Vår callback-funktion anropas en gång för varje punkt längs en linje
mellan musens föregående och nuvarande position, när den dras.

Vår callback uppmärksammar 'som' för att avgöra vilket av pluginets
verktyg som för närvarande är valt.
*/
void example_line_callback(void *pekare, int som, SDL_Surface *malarduk,
                           SDL_Surface *ogonblicksbild, int x, int y)
{
  /*
     Av tekniska skäl kan vi inte ta emot en pekare till Rita med Tux API:s
     struktur "magic_api", som de andra funktionerna gör.

     Istället får vi en "generisk" pekare (en "void *"). Raden nedan
     deklarerar en lokal "magic_api"-pekarvariabel som heter "api" och
     tilldelar den sedan värdet på den "generiska" pekaren som vi fick.

     "(magic_api *)" nedan kastar den generiska "void *"-pekaren till den
     "typ" av pekare vi vill ha, en pekare till en "magic_api"-struktur)
   */
  magic_api *api = (magic_api *) pekare;
  int xx, yy;

  /*
     Den här funktionen hanterar båda våra verktyg, så vi måste
     kontrollera vilket som används just nu.  Vi jämför argumentet 'som'
     som Rita med Tux skickar till oss med de värden vi räknade upp ovan.
   */

  if (som == VERKTYG_ONE)
  {
    /*
       Verktyg nummer 1 ritar helt enkelt en enda pixel på (x,y)-platsen. Det
       fungerar som en 1x1 pixel-pensel.
     */

    api->putpixel(malarduk, x, y,
                  SDL_MapRGB(malarduk->format,
                             example_r, example_g, example_b));

    /*
       Vi använder "SDL_MapRGB()" för att konvertera RGB-värdet som vi får
       från Rita med Tux för användarens aktuella färgval till ett
       "Uint32"-pixelvärde som vi kan skicka till Rita med Tuxs
       "putpixel()"-funktion.
     */
  }
  else if (som == VERKTYG_TWO)
  {
    /*
       Verktyg nummer 2 kopierar en kvadrat med pixlar (av den storlek som
       användaren väljer) från den motsatta sidan av duken och placerar den
       under markören.
     */

    for (yy = -example_storlek; yy < example_storlek; yy++)
    {
      for (xx = -example_storlek; xx < example_storlek; xx++)
      {
        api->putpixel(malarduk, x + xx, y + yy,
                      api->getpixel(ogonblicksbild,
                                    ogonblicksbild->w - x - xx,
                                    ogonblicksbild->h - y - yy));

        /*
           Här har vi helt enkelt använt Rita med Tuxs "getpixel()"-rutin för
           att hämta pixelvärden från "snapshot" och sedan "putpixel()" för att
           rita dem direkt i "canvas".

           Obs: putpixel() och getpixel() är säkra att använda även om dina
           X,Y-värden ligger utanför SDL-ytan (t.ex. negativa eller större än
           ytans bredd och/eller höjd).
         */
      }
    }
  }
}

/*
Switch-In händelse

Detta händer när ett magiskt verktyg är aktiverat, antingen för att
användaren just har valt det eller för att användaren återvände till
"Magic" efter att ha använt ett annat verktyg (t.ex. Brush eller Text) och
detta var det senast valda magiska verktyget.

(Detta gäller även momentana verktyg som Undo och Redo och
bildförändrande verktyg som New och Open)

Det händer också när ett Magic-verktygs läge ändras (vi kommer först
att få ett anrop till 'example_switchout()', nedan, för det gamla
läget).

Vårt exempel gör ingenting när vi byter till, eller från, våra
Magic-verktyg, så vi gör ingenting här.
*/
void example_switchin(magic_api *api, int som, int lage,
                      SDL_Surface *malarduk)
{
}

/*
Händelse för avstängning

Detta händer när ett magiskt verktyg inaktiveras, antingen för att
användaren har valt ett annat magiskt verktyg eller för att användaren
har valt ett helt annat verktyg (t.ex. Brush eller Text).

(Detta gäller även momentana verktyg som Undo och Redo och
bildförändrande verktyg som New och Open)

(Och i så fall kommer vår funktion example_switchin() att anropas en
stund senare)

Det händer också när ett Magic-verktygs läge ändras (vi får då ett
anrop till 'example_switchin()', ovan, för det nya läget).

Vårt exempel gör ingenting när vi byter till, eller från, våra
Magic-verktyg, så vi gör ingenting här.
*/
void example_switchout(magic_api *api, int som, int lage,
                       SDL_Surface *malarduk)
{
}
