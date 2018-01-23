/*
  i18n.h

  For Tux Paint
  Language-related functions

  Copyright (c) 2002-2012 by Bill Kendrick and others
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  (See COPYING.txt)

  $Id$
  
  June 14, 2002 - December 11, 2016
*/


#ifndef I18N_H
#define I18N_H

#include <stdio.h>
#include "compiler.h"

/* Possible languages: */

enum
{
  LANG_ACH,                     /* Acholi */
  LANG_AF,                      /* Afrikaans */
  LANG_AK,                      /* Akan */
  LANG_AM,                      /* Amharic */
  LANG_AN,                      /* Aragones */
  LANG_AR,                      /* Arabic */
  LANG_AS,                      /* Assamese */
  LANG_AST,                     /* Asturian */
  LANG_AZ,                      /* Azerbaijani */
  LANG_BE,                      /* Belarusian */
  LANG_BG,                      /* Bulgarian */
  LANG_BM,                      /* Bambara */
  LANG_BN,                      /* Bengali */
  LANG_BO,                      /* Tibetan */
  LANG_BR,                      /* Breton */
  LANG_BRX,                     /* Bodo */
  LANG_BS,                      /* Bosnian */
  LANG_CA_VALENCIA,             /* Valencian */
  LANG_CA,                      /* Catalan */
  LANG_CGG,                     /* Kiga */
  LANG_CS,                      /* Czech */
  LANG_CY,                      /* Welsh */
  LANG_DA,                      /* Danish */
  LANG_DE,                      /* German */
  LANG_DOI,                     /* Dogri */
  LANG_EL,                      /* Greek */
  LANG_EN,                      /* English (American) (DEFAULT) */
  LANG_EN_AU,                   /* English (Australian) */
  LANG_EN_CA,                   /* English (Canadian) */
  LANG_EN_GB,                   /* English (British) */
  LANG_EN_ZA,                   /* English (South African) */
  LANG_EO,                      /* Esperanto */
  LANG_ES_MX,                   /* Spanish (Mexican) */
  LANG_ES,                      /* Spanish */
  LANG_ET,                      /* Estonian */
  LANG_EU,                      /* Basque */
  LANG_FA,                      /* Persian */
  LANG_FF,                      /* Fulah */
  LANG_FI,                      /* Finnish */
  LANG_FO,                      /* Faroese */
  LANG_FR,                      /* French */
  LANG_GA,                      /* Irish Gaelic */
  LANG_GD,                      /* Scottish Gaelic */
  LANG_GL,                      /* Galician */
  LANG_GR,                      /* Gronings */
  LANG_GU,                      /* Gujarati */
  LANG_HE,                      /* Hebrew */
  LANG_HI,                      /* Hindi */
  LANG_HR,                      /* Croatian */
  LANG_HU,                      /* Hungarian */
  LANG_HY,                      /* Armenian */
  LANG_I_KLINGON_ROMANIZED,     /* Klingon (Romanized) */
  LANG_ID,                      /* Indonesian */
  LANG_IS,                      /* Icelandic */
  LANG_IT,                      /* Italian */
  LANG_IU,                      /* Inuktitut */
  LANG_JA,                      /* Japanese */
  LANG_KA,                      /* Georgian */
  LANG_KAB,                     /* Kabyle */
  LANG_KN,                      /* Kannada */
  LANG_KM,                      /* Khmer */
  LANG_KOK_ROMAN,               /* Konkani (Roman) */
  LANG_KOK,                     /* Konkani (Devaganari) */
  LANG_KO,                      /* Korean */
  LANG_KS_DEVANAGARI,           /* Kashmiri (Devanagari script) */
  LANG_KS,                      /* Kashmiri (Perso-Arabic script) */
  LANG_KU,                      /* Kurdish */
  LANG_LB,                      /* Luxembourgish */
  LANG_LG,                      /* Luganda */
  LANG_LT,                      /* Lithuanian */
  LANG_LV,                      /* Latvian */
  LANG_MAI,                     /* Maithili */
  LANG_ML,                      /* Malayalam */
  LANG_MK,                      /* Macedonian */
  LANG_MN,                      /* Mongolian */
  LANG_MNI_BENGALI,             /* Manipuri (Bengali script) */
  LANG_MNI_METEI_MAYEK,         /* Manipuri (Metei Mayek script) */
  LANG_MR,                      /* Marath */
  LANG_MS,                      /* Malay */
  LANG_NB,                      /* Norwegian Bokmal */
  LANG_NE,                      /* Nepali */
  LANG_NL,                      /* Dutch */
  LANG_NN,                      /* Norwegian Nynorsk */
  LANG_NR,                      /* Ndebele */
  LANG_NSO,                     /* Northern Sotho */
  LANG_OC,                      /* Occitan */
  LANG_OJ,                      /* Ojibway */
  LANG_OR,                      /* Odia */
  LANG_PA,                      /* Punjabi */
  LANG_PL,                      /* Polish */
  LANG_PT_BR,                   /* Portuguese (Brazilian) */
  LANG_PT,                      /* Portuguese */
  LANG_RO,                      /* Romanian */
  LANG_RU,                      /* Russian */
  LANG_RW,                      /* Kinyarwanda */
  LANG_SAT_OL_CHIKI,
  LANG_SAT,                     /* Santali */
  LANG_SA,                      /* Sanskrit */
  LANG_SD,                      /* Sindhi (Perso-Arabic) */
  LANG_SD_DEVANAGARI,           /* Sindhi (Devanagari) */
  LANG_SHS,                     /* Shuswap */
  LANG_SI,                      /* Sinhala */
  LANG_SK,                      /* Slovak */
  LANG_SL,                      /* Slovenian */
  LANG_SON,                     /* Songhay */
  LANG_SQ,                      /* Albanian */
  LANG_SR_LATIN,                /* Serbian (latin) */
  LANG_SR,                      /* Serbian (cyrillic) */
  LANG_SU,                      /* Sundanese */
  LANG_SV,                      /* Swedish */
  LANG_SW,                      /* Swahili */
  LANG_TA,                      /* Tamil */
  LANG_TE,                      /* Telugu */
  LANG_TH,                      /* Thai */
  LANG_TL,                      /* Tagalog */
  LANG_TR,                      /* Turkish */
  LANG_TW,                      /* Twi */
  LANG_UK,                      /* Ukrainian */
  LANG_UR,                      /* Urdu */
  LANG_VEC,                     /* Venetian */
  LANG_VE,                      /* Venda */
  LANG_VI,                      /* Vietnamese */
  LANG_WA,                      /* Walloon */
  LANG_WO,                      /* Wolof */
  LANG_XH,                      /* Xhosa */
  LANG_ZAM,                     /* Zapotec (Miahuatlan) */
  LANG_ZH_CN,                   /* Chinese (Simplified) */
  LANG_ZH_TW,                   /* Chinese (Traditional) */
  LANG_ZU,                      /* Zulu */
  NUM_LANGS
};


/* Types: */

typedef struct language_to_locale_struct
{
  const char *language;
  const char *locale;
} language_to_locale_struct;


/* Globals: */

extern const char *lang_prefixes[NUM_LANGS];
extern int need_own_font;
extern int need_right_to_left;  // Right-justify
extern int need_right_to_left_word;     // Words need to be reversed, too! (e.g., Hebrew, but not Arabic)
extern const char *lang_prefix, *short_lang_prefix;
extern int num_wished_langs;

typedef struct w_langs
{
  int langint;
  int need_own_font;
  int need_right_to_left;
  int need_right_to_left_word;
  int lang_y_nudge;
  const char *lang_prefix;
  const char *short_lang_prefix;
} w_langs;

extern w_langs wished_langs[255];

/* Function prototypes: */

int get_current_language(void);
int setup_i18n(const char *restrict lang, const char *restrict locale) MUST_CHECK;

#ifdef NO_SDLPANGO
     int smash_i18n(void) MUST_CHECK;
#endif

#endif
