/*
  i18n.c

  For Tux Paint
  Language-related functions

  Copyright (c) 2002-2023 by Bill Kendrick and others
  bill@newbreedsoftware.com
  https://tuxpaint.org/

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

  June 14, 2002 - June 2, 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include "platform.h"
#include "i18n.h"
#include "debug.h"

#ifdef WIN32
#include <sys/types.h>
#endif

#ifdef BDIST_WIN32
#include <unistd.h>
#endif

#ifdef __BEOS__
#include <wchar.h>
#else
#include <wchar.h>
#include <wctype.h>
#endif

#ifdef __ANDROID__
#include "SDL2/SDL.h"
#include "../../../SDL2/src/core/android/SDL_android.h"
#include "jni.h"
// since setlocale on the Android is not supported well,
// setlocale cannot get current default locale of the device.
// Here, JNI and Java Locale class can get the default locale
// if user has not set locale and lang in the config file yet.
static char *android_locale()
{
  static char android_locale_buf[32];
  JNIEnv *mEnv = Android_JNI_GetEnv();
  jclass mLocaleClass = (*mEnv)->FindClass(mEnv, "java/util/Locale");
  jmethodID mGetDefaultMethod = (*mEnv)->GetStaticMethodID(mEnv, mLocaleClass, "getDefault",
                                                           "()Ljava/util/Locale;");
  jobject mLocaleObject = (*mEnv)->CallStaticObjectMethod(mEnv, mLocaleClass, mGetDefaultMethod);
  jmethodID mToStringMethod = (*mEnv)->GetMethodID(mEnv, mLocaleClass, "toString",
                                                   "()Ljava/lang/String;");
  jstring mLocaleString = (*mEnv)->CallObjectMethod(mEnv, mLocaleObject, mToStringMethod);
  const char *locale = (*mEnv)->GetStringUTFChars(mEnv, mLocaleString, 0);

  strcpy(android_locale_buf, locale);
  (*mEnv)->ReleaseStringUTFChars(mEnv, mLocaleString, locale);
  printf("android locale %s\n", android_locale_buf);
  return android_locale_buf;
}
#endif
#if defined(__MACOS__)
#include "macos.h"
#elif defined(__IOS__)
#include "ios.h"
#endif


/* Globals: */

static int langint = LANG_EN;

/* Strings representing each language's ISO 639 (-1 or -2) codes.
 * Should map to the 'enum' of possible languages ("LANG_xxx")
 * found in "i18n.h" (where "NUM_LANGS" is found, as the final
 * entry in the 'enum' list). */
const char *lang_prefixes[NUM_LANGS] = {
  "ach",
  "af",
  "ak",
  "am",
  "an",
  "ar",
  "as",
  "ast",
  "az",
  "be",
  "bg",
  "bm",
  "bn",
  "bo",
  "br",
  "brx",
  "bs",
  "ca@valencia",
  "ca",
  "cgg",
  "cs",
  "cy",
  "da",
  "de",
  "doi",
  "el",
  "en",
  "en_AU",
  "en_CA",
  "en_GB",
  "en_ZA",
  "eo",
  "es_MX",
  "es",
  "et",
  "eu",
  "fa",
  "ff",
  "fi",
  "fo",
  "fr",
  "ga",
  "gd",
  "gl",
  "gos",
  "gu",
  "he",
  "hi",
  "hr",
  "hu",
  "hy",
  "tlh",
  "id",
  "is",
  "it",
  "iu",
  "ja",
  "ka",
  "kab",
  "kn",
  "km",
  "kok@roman",
  "kok",
  "ko",
  "ks@devanagari",
  "ks",
  "ku",
  "lb",
  "lg",
  "lt",
  "lv",
  "mai",
  "ml",
  "mk",
  "mn",
  "mni",
  "mni@meiteimayek",
  "mr",
  "ms",
  "nb",
  "ne",
  "nl",
  "nn",
  "nr",
  "nso",
  "oc",
  "oj",
  "or",
  "pa",
  "pl",
  "pt_BR",
  "pt",
  "ro",
  "ru",
  "rw",
  "sat@olchiki",
  "sat",
  "sa",
  "sc",
  "sd",
  "sd@devanagari",
  "shs",
  "si",
  "sk",
  "sl",
  "son",
  "sq",
  "sr@latin",
  "sr",
  "su",
  "sv",
  "sw",
  "ta",
  "te",
  "th",
  "tl",
  "tr",
  "tw",
  "uk",
  "ur",
  "vec",
  "ve",
  "vi",
  "wa",
  "wo",
  "xh",
  "zam",
  "zh_CN",
  "zh_TW",
  "zu",
};


/* Languages which don't use the default font */
static int lang_use_own_font[] = {
  LANG_AR,
  LANG_BO,
  LANG_GU,
  LANG_HI,
  LANG_JA,
  LANG_KA,
  LANG_KO,
  LANG_ML,
  LANG_TA,
  LANG_TE,
  LANG_TH,
  LANG_ZH_CN,
  LANG_ZH_TW,
  -1
};

/* Languages which are written right-to-left */
static int lang_use_right_to_left[] = {
  LANG_AR,
  LANG_FA,
  LANG_HE,
  LANG_KS,
  LANG_SD,
  LANG_UR,
  -1
};

int need_own_font;
int need_right_to_left;
const char *lang_prefix, *short_lang_prefix;

w_langs wished_langs[255];

/* Mappings from human-readable language names (found in
 * config files, or command-line arguments) to the precise
 * local code to use.  Some locales appear multiple times,
 * (e.g. "de_DE.UTF-8" is represented by both "german"
 * (the English name of the language) and "deutsch"
 * (the German name of the language)).
 */
static const language_to_locale_struct language_to_locale_array[] = {
  {"english", "C"},
  {"american-english", "C"},
  {"acholi", "ach_UG.UTF-8"},
  {"acoli", "ach_UG.UTF-8"},
  {"akan", "ak_GH.UTF-8"},
  {"twi-fante", "ak_GH.UTF-8"},
  {"amharic", "am_ET.UTF-8"},
  {"arabic", "ar_SA.UTF-8"},
  {"aragones", "an_ES.UTF-8"},
  {"armenian", "hy_AM.UTF-8"},
  {"hayeren", "hy_AM.UTF-8"},
  {"assamese", "as_IN.UTF-8"},
  {"asturian", "ast_ES.UTF-8"},
  {"azerbaijani", "az_AZ.UTF-8"},
  {"bambara", "bm_ML.UTF-8"},
  {"bengali", "bn_IN.UTF-8"},
  {"bodo", "brx_IN.UTF-8"},
  {"bosnian", "bs_BA.UTF-8"},
  {"croatian", "hr_HR.UTF-8"},
  {"hrvatski", "hr_HR.UTF-8"},
  {"catalan", "ca_ES.UTF-8"},
  {"catala", "ca_ES.UTF-8"},
  {"valencian", "ca_ES.UTF-8@valencia"},
  {"valencia", "ca_ES.UTF-8@valencia"},
  {"kiga", "cgg_UG.UTF-8"},
  {"chiga", "cgg_UG.UTF-8"},
  {"belarusian", "be_BY.UTF-8"},
  {"bielaruskaja", "be_BY.UTF-8"},
  {"czech", "cs_CZ.UTF-8"},
  {"cesky", "cs_CZ.UTF-8"},
  {"danish", "da_DK.UTF-8"},
  {"dansk", "da_DK.UTF-8"},
  {"dogri", "doi_IN.UTF-8"},
  {"german", "de_DE.UTF-8"},
  {"deutsch", "de_DE.UTF-8"},
  {"estonian", "et_EE.UTF-8"},
  {"greek", "el_GR.UTF-8"},
  {"gronings", "gos_NL.UTF-8"},
  {"zudelk-veenkelonioals", "gos_NL.UTF-8"},
  {"gujarati", "gu_IN.UTF-8"},
  {"british-english", "en_GB.UTF-8"},
  {"british", "en_GB.UTF-8"},
  {"australian-english", "en_AU.UTF-8"},
  {"canadian-english", "en_CA.UTF-8"},
  {"southafrican-english", "en_ZA.UTF-8"},
  {"esperanto", "eo.UTF-8"},
  {"spanish", "es_ES.UTF-8"},
  {"mexican", "es_MX.UTF-8"},
  {"mexican-spanish", "es_MX.UTF-8"},
  {"espanol-mejicano", "es_MX.UTF-8"},
  {"espanol", "es_ES.UTF-8"},
  {"persian", "fa_IR.UTF-8"},
  {"fula", "ff_SN.UTF-8"},
  {"fulah", "ff_SN.UTF-8"},
  {"pulaar-fulfulde", "ff_SN.UTF-8"},
  {"finnish", "fi_FI.UTF-8"},
  {"suomi", "fi_FI.UTF-8"},
  {"faroese", "fo_FO.UTF-8"},
  {"french", "fr_FR.UTF-8"},
  {"francais", "fr_FR.UTF-8"},
  {"gaelic", "ga_IE.UTF-8"},
  {"irish-gaelic", "ga_IE.UTF-8"},
  {"gaidhlig", "ga_IE.UTF-8"},
  {"scottish", "gd_GB.UTF-8"},
  {"ghaidhlig", "gd_GB.UTF-8"},
  {"scottish-gaelic", "gd_GB.UTF-8"},
  {"galician", "gl_ES.UTF-8"},
  {"galego", "gl_ES.UTF-8"},
  {"hebrew", "he_IL.UTF-8"},
  {"hindi", "hi_IN.UTF-8"},
  {"hungarian", "hu_HU.UTF-8"},
  {"magyar", "hu_HU.UTF-8"},
  {"indonesian", "id_ID.UTF-8"},
  {"bahasa-indonesia", "id_ID.UTF-8"},
  {"icelandic", "is_IS.UTF-8"},
  {"islenska", "is_IS.UTF-8"},
  {"italian", "it_IT.UTF-8"},
  {"italiano", "it_IT.UTF-8"},
  {"inuktitut", "iu_CA.UTF-8"},
  {"japanese", "ja_JP.UTF-8"},
  {"venda", "ve_ZA.UTF-8"},
  {"venetian", "vec.UTF-8"},
  {"veneto", "vec.UTF-8"},
  {"vietnamese", "vi_VN.UTF-8"},
  {"afrikaans", "af_ZA.UTF-8"},
  {"albanian", "sq_AL.UTF-8"},
  {"breton", "br_FR.UTF-8"},
  {"brezhoneg", "br_FR.UTF-8"},
  {"bulgarian", "bg_BG.UTF-8"},
  {"welsh", "cy_GB.UTF-8"},
  {"cymraeg", "cy_GB.UTF-8"},
  {"bokmal", "nb_NO.UTF-8"},
  {"basque", "eu_ES.UTF-8"},
  {"euskara", "eu_ES.UTF-8"},
  {"georgian", "ka_GE"},
  {"kabyle", "kab"},
  {"kabylian", "kab"},
  {"kinyarwanda", "rw_RW.UTF-8"},
  {"klingon", "tlh.UTF-8"},
  {"tlhIngan", "tlh.UTF-8"},
  {"kannada", "kn_IN.UTF-8"},
  {"korean", "ko_KR.UTF-8"},
  {"kashmiri-devanagari", "ks_IN.UTF-8@devanagari"},
  {"kashmiri-perso-arabic", "ks_IN.UTF-8"},
  {"kurdish", "ku_TR.UTF-8"},
  {"tamil", "ta_IN.UTF-8"},
  {"telugu", "te_IN.UTF-8"},
  {"lithuanian", "lt_LT.UTF-8"},
  {"lietuviu", "lt_LT.UTF-8"},
  {"latvian", "lv_LV.UTF-8"},
  {"luganda", "lg_UG.UTF-8"},
  {"luxembourgish", "lb_LU.UTF-8"},
  {"letzebuergesch", "lb_LU.UTF-8"},
  {"konkani-devanagari", "kok_IN.UTF-8"},
  {"konkani-roman", "kok@roman"},
  {"maithili", "mai_IN.UTF-8"},
  {"macedonian", "mk_MK.UTF-8"},
  {"mongolian", "mn_MN.UTF-8"},
  {"manipuri-bengali", "mni_IN.UTF-8"},
  {"manipuri-meitei-mayek", "mni@meiteimayek"},
  {"marathi", "mr_IN.UTF-8"},
  {"malay", "ms_MY.UTF-8"},
  {"nepali", "ne_NP.UTF-8"},
  {"dutch", "nl_NL.UTF-8"},
  {"nederlands", "nl_NL.UTF-8"},
  {"norwegian", "nn_NO.UTF-8"},
  {"nynorsk", "nn_NO.UTF-8"},
  {"norsk", "nn_NO.UTF-8"},
  {"ndebele", "nr_ZA.UTF-8"},
  {"northern-sotho", "nso_ZA.UTF-8"},
  {"sesotho-sa-leboa", "nso_ZA.UTF-8"},
  {"occitan", "oc_FR.UTF-8"},
  {"odia", "or_IN.UTF-8"},      // Proper spelling
  {"oriya", "or_IN.UTF-8"},     // Alternative
  {"ojibwe", "oj_CA.UTF-8"},    // Proper spelling
  {"ojibway", "oj_CA.UTF-8"},   // For compatibility
  {"punjabi", "pa_IN.UTF-8"},
  {"panjabi", "pa_IN.UTF-8"},
  {"polish", "pl_PL.UTF-8"},
  {"polski", "pl_PL.UTF-8"},
  {"brazilian-portuguese", "pt_BR.UTF-8"},
  {"portugues-brazilian", "pt_BR.UTF-8"},
  {"brazilian", "pt_BR.UTF-8"},
  {"portuguese", "pt_PT.UTF-8"},
  {"portugues", "pt_PT.UTF-8"},
  {"romanian", "ro_RO.UTF-8"},
  {"russian", "ru_RU.UTF-8"},
  {"russkiy", "ru_RU.UTF-8"},
  {"sanskrit", "sa_IN.UTF-8"},
  {"santali-devanagari", "sat_IN.UTF-8"},
  {"santali-ol-chiki", "sat@olchiki"},
  {"sardinian", "sc_IT"},
  {"sardu", "sc_IT"},
  {"serbian", "sr_RS.UTF-8"},   /* Was sr_YU, but that's not in /usr/share/i18n/SUPPORTED, and sr_RS is -bjk 2014.08.04 */
  {"serbian-latin", "sr_RS@latin"},
  {"shuswap", "shs_CA.UTF-8"},
  {"secwepemctin", "shs_CA.UTF-8"},
  {"sindhi-perso-arabic", "sd_IN.UTF-8"},
  {"sindhi-devanagari", "sd_IN.UTF-8@devanagari"},
  {"sinhala", "si_LK.UTF-8"},
  {"slovak", "sk_SK.UTF-8"},
  {"slovenian", "sl_SI.UTF-8"},
  {"slovensko", "sl_SI.UTF-8"},
  {"songhay", "son.UTF-8"},
  {"sundanese", "su_ID.UTF-8"},
  {"swedish", "sv_SE.UTF-8"},
  {"svenska", "sv_SE.UTF-8"},
  {"swahili", "sw_TZ.UTF-8"},
  {"tagalog", "tl_PH.UTF-8"},
  {"thai", "th_TH.UTF-8"},
  {"tibetan", "bo_CN.UTF-8"},   /* Based on: http://texinfo.org/pipermail/texinfo-pretest/2005-April/000334.html */
  {"turkish", "tr_TR.UTF-8"},
  {"twi", "tw_GH.UTF-8"},
  {"ukrainian", "uk_UA.UTF-8"},
  {"urdu", "ur_IN.UTF-8"},
  {"walloon", "wa_BE.UTF-8"},
  {"walon", "wa_BE.UTF-8"},
  {"wolof", "wo_SN.UTF-8"},
  {"xhosa", "xh_ZA.UTF-8"},
  {"chinese", "zh_CN.UTF-8"},
  {"simplified-chinese", "zh_CN.UTF-8"},
  {"traditional-chinese", "zh_TW.UTF-8"},
  {"zapotec", "zam.UTF-8"},
  {"miahuatlan-zapotec", "zam.UTF-8"},
  {"khmer", "km_KH.UTF-8"},
  {"malayalam", "ml_IN.UTF-8"},
  {"zulu", "zu_ZA.UTF-8"}
};


/**
 * Show available languages
 *
 * @param exitcode Exit code; also determines whether STDERR or STDOUT used.
 *   (e.g., is this output of "--lang help" (STDOUT & exit 0),
 *   or complaint of an inappropriate "--lang" argument (STDERR & exit 1)?)
 */
static void show_lang_usage(int exitcode)
{
  FILE *f = exitcode ? stderr : stdout;
  const char *const prg = "tuxpaint";

  /* FIXME: All this should REALLY be array-based!!! */
  fprintf(f, "\n" "Usage: %s [--lang LANGUAGE]\n" "\n" "LANGUAGE may be one of:\n"
/* C */ "  english      american-english\n"
/* ach */ "  acholi       acoli\n"
/* af */ "  afrikaans\n"
/* ak */ "  akan         twi-fante\n"
/* sq */ "  albanian\n"
/* am */ "  amharic\n"
/* ar */ "  arabic\n"
/* an */ "  aragones\n"
/* hy */ "  armenian     hayeren\n"
/* as */ "  assamese\n"
/* ast */ "  asturian\n"
/* en_AU */ "  australian-english\n"
/* az */ "  azerbaijani\n"
/* bm */ "  bambara\n"
/* eu */ "  basque       euskara\n"
/* be */ "  belarusian   bielaruskaja\n"
/* bn */ "  bengali\n"
/* brx */ "  bodo\n"
/* nb */ "  bokmal\n"
/* bs */ "  bosnian\n"
/* pt_BR */
          "  brazilian    brazilian-portuguese   portugues-brazilian\n"
/* br */ "  breton       brezhoneg\n"
/* en_GB */ "  british      british-english\n"
/* bg_BG */ "  bulgarian\n"
/* en_CA */ "  canadian-english\n"
/* ca */ "  catalan      catala\n"
/* zh_CN */ "  chinese      simplified-chinese\n"
/* zh_TW */ "               traditional-chinese\n"
/* hr */ "  croatian     hrvatski\n"
/* cs */ "  czech        cesky\n"
/* da */ "  danish       dansk\n"
/* doi */ "  dogri\n"
/* nl */ "  dutch        nederlands\n"
/* eo */ "  esperanto\n"
/* et */ "  estonian\n"
/* fo */ "  faroese\n"
/* fi */ "  finnish      suomi\n"
/* fr */ "  french       francais\n"
/* ff */ "  fula         fulah                  pulaar-fulfulde\n"
/* ga */ "  gaelic       irish-gaelic           gaidhlig\n"
/* gl */ "  galician     galego\n"
/* ka */ "  georgian\n"
/* de */ "  german       deutsch\n"
/* el */ "  greek\n"
/* gos */ "  gronings     zudelk-veenkelonioals\n"
/* gu */ "  gujarati\n"
/* he */ "  hebrew\n"
/* hi */ "  hindi\n"
/* hu */ "  hungarian    magyar\n"
/* is */ "  icelandic    islenska\n"
/* id */ "  indonesian   bahasa-indonesia\n"
/* iu */ "  inuktitut\n"
/* it */ "  italian      italiano\n"
/* ja */ "  japanese\n"
/* kab */ "  kabyle       kabylian\n"
/* kn */ "  kannada\n"
/* ks@devanagari */ "  kashmiri-devanagari\n"
/* ks */ "  kashmiri-perso-arabic\n"
/* km */ "  khmer\n"
/* cgg */ "  kiga         chiga\n"
/* rw */ "  kinyarwanda\n"
/* tlh */ "  klingon      tlhIngan\n"
/* kok */ "  konkani-devanagari\n"
/* kok@roman */ "  konkani-roman\n"
/* ko */ "  korean\n"
/* ku */ "  kurdish\n"
/* lv */ "  latvian\n"
/* lt */ "  lithuanian   lietuviu\n"
/* lg */ "  luganda\n"
/* lb */ "  luxembourgish letzebuergesch\n"
/* mai */ "  maithili\n"
/* mk */ "  macedonian\n"
/* ms */ "  malay\n"
/* ml */ "  malayalam\n"
/* mni */ "  manipuri-bengali\n"
/* mni@meiteimayek */ "  manipuri-meitei-mayek\n"
/* nr */ "  marathi\n"
/* es_MX */
          "  mexican      mexican-spanish        espanol-mejicano\n"
/* mn */ "  mongolian\n"
/* nr */ "  ndebele\n"
/* ne */ "  nepali\n"
/* nso */ "  northern-sotho                      sesotho-sa-leboa\n"
/* nn */ "  norwegian    nynorsk                norsk\n"
/* oc */ "  occitan\n"
/* or */ "  odia         oriya\n"
/* oj */ "  ojibwe       ojibway\n"
/* fa */ "  persian\n"
/* pl */ "  polish       polski\n"
/* pt */ "  portuguese   portugues\n"
/* pa */ "  punjabi      panjabi\n"
/* ro */ "  romanian\n"
/* ru */ "  russian      russkiy\n"
/* sa */ "  sanskrit\n"
/* sat */ "  santali-devanagari\n"
/* sat@olchiki */ "  santali-ol-chiki\n"
/* sc */ "  sardinian    sardu\n"
/* gd */ "  scottish     scottish-gaelic        ghaidhlig\n"
/* sr */ "  serbian\n"
/* sr@latin */ "  serbian-latin\n"
/* shs*/ "  shuswap      secwepemctin\n"
/* sd@devanagari */ "  sindhi-devanagari\n"
/* sd */ "  sindhi-perso-arabic\n"
/* si */ "  sinhala\n"
/* sk */ "  slovak\n"
/* sl */ "  slovenian    slovensko\n"
/* en_ZA */ "  southafrican-english\n"
/* son */ "  songhay\n"
/* es */ "  spanish      espanol\n"
/* su */ "  sundanese\n"
/* sw */ "  swahili\n"
/* sv */ "  swedish      svenska\n"
/* tl */ "  tagalog\n"
/* ta */ "  tamil\n"
/* te */ "  telugu\n"
/* th */ "  thai\n"
/* twi */ "  twi\n"
/* bo */ "  tibetan\n"
/* tr */ "  turkish\n"
/* uk */ "  ukrainian\n"
/* ur */ "  urdu\n"
/* ca@valencia */ "  valencian    valencia\n"
/* ve */ "  venda\n"
/* vec */ "  venetian     veneto\n"
/* vi */ "  vietnamese\n"
/* wa */ "  walloon      walon\n"
/* wo */ "  wolof\n"
/* cy */ "  welsh        cymraeg\n"
/* xh */ "  xhosa\n"
/* zam */ "  zapotec      miahuatlan-zapotec\n"
/* zu */ "  zulu\n"
          "\n", prg);
  exit(exitcode);
}


/**
 * Show available locales as a "usage" output
 *
 * @param f File descriptor to write to (e.g., STDOUT or STDERR)
 * @param prg Program name (e.g., "tuxpaint" or "tuxpaint.exe")
 */
static void show_locale_usage(FILE *f, const char *const prg)
{
  /* FIXME: Add accented characters to the descriptions */
  fprintf(f,
          "\n"
          "Usage: %s [--locale LOCALE]\n"
          "\n"
          "LOCALE may be one of:\n"
          "  C       (English      American English)\n"
          "  ach_UG  (Acholi       Acoli)\n"
          "  af_ZA   (Afrikaans)\n"
          "  ak_GH   (Akan         Twi-Fante)\n"
          "  am_ET   (Amharic)\n"
          "  ar_SA   (Arabic)\n"
          "  an_ES   (Aragones)\n"
          "  hy_AM   (Armenian)\n"
          "  as_IN   (Assamese)\n"
          "  ast_ES  (Asturian)\n"
          "  az_AZ   (Azerbaijani)\n"
          "  bm_ML   (Bambara)\n"
          "  eu_ES   (Basque       Euskara)\n"
          "  be_BY   (Belarusian   Bielaruskaja)\n"
          "  bn_IN   (Bengali)\n"
          "  brx_IN  (Bodo)\n"
          "  bs_BA   (Bosnian)\n"
          "  nb_NO   (Bokmal)\n"
          "  pt_BR   (Brazilian    Brazilian Portuguese   Portugues Brazilian)\n"
          "  br_FR   (Breton       Brezhoneg)\n"
          "  en_AU   (Australian English)\n"
          "  en_CA   (Canadian English)\n"
          "  en_GB   (British      British English)\n"
          "  en_ZA   (South African English)\n"
          "  bg_BG   (Bulgarian)\n"
          "  ca_ES   (Catalan      Catala)\n"
          "  ca_ES@valencia   (Valencian    Valencia)n"
          "  zh_CN   (Chinese-Simplified)\n"
          "  zh_TW   (Chinese-Traditional)\n"
          "  cs_CZ   (Czech        Cesky)\n"
          "  da_DK   (Danish       Dansk)\n"
          "  doi_IN   (Dogri)\n"
          "  nl_NL   (Dutch)\n"
          "  fa_IR   (Persian)\n"
          "  ff_SN   (Fulah)\n"
          "  fi_FI   (Finnish      Suomi)\n"
          "  fo_FO   (Faroese)\n"
          "  fr_FR   (French       Francais)\n"
          "  ga_IE   (Irish Gaelic Gaidhlig)\n"
          "  gd_GB   (Scottish Gaelic  Ghaidhlig)\n"
          "  gl_ES   (Galician     Galego)\n"
          "  gos_NL  (Gronings     Zudelk Veenkelonioals)\n"
          "  gu_IN   (Gujarati)\n"
          "  de_DE   (German       Deutsch)\n"
          "  eo      (Esperanto)\n"
          "  et_EE   (Estonian)\n"
          "  el_GR   (Greek)\n"
          "  he_IL   (Hebrew)\n"
          "  hi_IN   (Hindi)\n"
          "  hr_HR   (Croatian     Hrvatski)\n"
          "  hu_HU   (Hungarian    Magyar)\n"
          "  cgg_UG  (Kiga         Chiga)\n"
          "  tlh     (Klingon      tlhIngan)\n"
          "  is_IS   (Icelandic    Islenska)\n"
          "  id_ID   (Indonesian   Bahasa Indonesia)\n"
          "  it_IT   (Italian      Italiano)\n"
          "  iu_CA   (Inuktitut)\n"
          "  ja_JP   (Japanese)\n"
          "  ka_GE   (Georgian)\n"
          "  kn_IN   (Kannada)\n"
          "  km_KH   (Khmer)\n"
          "  ko_KR   (Korean)\n"
          "  ks_IN@devanagari   (Kashmiri (Devanagari))\n"
          "  ks_IN   (Kashmiri (Perso-Arabic))\n"
          "  ku_TR   (Kurdish)\n"
          "  ms_MY   (Malay)\n"
          "  ml_IN   (Malayalam)\n"
          "  lg_UG   (Luganda)\n"
          "  lb_LU   (Luxembourgish Letzebuergesch)\n"
          "  lv_LV   (Latvian)\n"
          "  lt_LT   (Lithuanian   Lietuviu)\n"
          "  kok_IN  (Konkani (Devanagari))\n"
          "  kok@roman  (Konkani (Roman))\n"
          "  mai_IN  (Maithili)\n"
          "  mk_MK   (Macedonian)\n"
          "  mni_IN  (Manipuri (Bengali))\n"
          "  mni@meiteimayek (Manipuri(Meitei Mayek))\n"
          "  mn_MN   (Mongolian)\n"
          "  mr_IN   (Marathi)\n"
          "  nr_ZA   (Ndebele)\n"
          "  ne_NP   (Nepali)\n"
          "  nso_ZA  (Northern Sotho                      Sotho sa Leboa)\n"
          "  nn_NO   (Norwegian    Nynorsk                Norsk)\n"
          "  oc_FR   (Occitan)\n"
          "  oj_CA   (Ojibway)\n"
          "  or_IN   (Odia         Oriya)\n"
          "  pa_IN   (Punjabi      Panjabi)\n"
          "  pl_PL   (Polish       Polski)\n"
          "  pt_PT   (Portuguese   Portugues)\n"
          "  ro_RO   (Romanian)\n"
          "  ru_RU   (Russian      Russkiy)\n"
          "  rw_RW   (Kinyarwanda)\n"
          "  sa_IN   (Sanskrit)\n"
          "  sat_IN  (Santali)\n"
          "  sat@olchiki  (Santali (Ol-Chiki))\n"
          "  sc_IT   (Sardinian)\n"
          "  sd_IN@devanagari  (Sindhi (Devanagari))\n"
          "  sd_IN  (Sindhii (Perso-Arabic))\n"
          "  shs_CA  (Shuswap      Secwepemctin)\n"
          "  si_LK   (Sinhala)\n"
          "  sk_SK   (Slovak)\n"
          "  sl_SI   (Slovenian)\n"
          "  son     (Songhay)\n"
          "  sq_AL   (Albanian)\n"
          "  sr_YU   (Serbian (cyrillic))\n"
          "  sr_RS@latin  (Serbian (latin))\n"
          "  es_ES   (Spanish      Espanol)\n"
          "  su_ID   (Sundanese)\n"
          "  es_MX   (Mexican      Mexican Spanish       Espanol Mejicano)\n"
          "  sw_TZ   (Swahili)\n"
          "  sv_SE   (Swedish      Svenska)\n"
          "  ta_IN   (Tamil)\n"
          "  te_IN   (Telugu)\n"
          "  tl_PH   (Tagalog)\n"
          "  bo_CN   (Tibetan)\n"
          "  th_TH   (Thai)\n"
          "  tr_TR   (Turkish)\n"
          "  tw_GH  (Twi)\n"
          "  uk_UA   (Ukrainian)\n"
          "  ur_IN   (Urdu)\n"
          "  ve_ZA   (Venda)\n"
          "  vec     (Venetian)\n"
          "  vi_VN   (Vietnamese)\n"
          "  wa_BE   (Walloon)\n"
          "  wo_SN   (Wolof)\n"
          "  cy_GB   (Welsh        Cymraeg)\n"
          "  xh_ZA   (Xhosa)\n" "  zam     (Zapoteco-Miahuatlan)\n" "  zu_ZA   (Zulu)\n" "\n", prg);
}

/**
 * Return the current language
 *
 * @return The current language (one of the LANG_xxx enums)
 */
int get_current_language(void)
{
  return langint;
}

/**
 * Search an array of ints for a given int
 *
 * @param l The int to search for
 * @param array The array of ints to search, terminated by -1
 * @return 1 if "l" is found in "array", 0 otherwise
 */
static int search_int_array(int l, int *array)
{
  int i;

  for (i = 0; array[i] != -1; i++)
  {
    if (array[i] == l)
      return 1;
  }

  return 0;
}

/**
 * Ensures that iswprint() works beyond ASCII,
 * even if the locale wouldn't normally support that.
 * Tries fallback locales until one works.
 * Emits an error message to STDERR if none work.
 */
static void ctype_utf8(void)
{
#ifndef _WIN32
  /* FIXME: should this iterate over more locales?
     A zapotec speaker may have es_MX.UTF-8 available but not have en_US.UTF-8 for example */
  const char *names[] = { "en_US.UTF8", "en_US.UTF-8", "UTF8", "UTF-8", "C.UTF-8" };
  int i = sizeof(names) / sizeof(names[0]);

  for (;;)
  {
    if (iswprint((wchar_t)0xf7))        // division symbol -- which is in Latin-1 :-/
      return;
    if (--i < 0)
      break;
    setlocale(LC_CTYPE, names[i]);
    setlocale(LC_MESSAGES, names[i]);
  }
  fprintf(stderr, "Failed to find a locale with iswprint() working!\n");
#endif
}

/**
 * For a given language, return its locale, or exit with a usage error.
 *
 * @param langstr Name of language (e.g., "german")
 * @return Locale (e.g., "de_DE.UTF-8")
 */
static const char *language_to_locale(const char *langstr)
{
  int i = sizeof language_to_locale_array / sizeof language_to_locale_array[0];

  while (i--)
  {
    if (!strcmp(langstr, language_to_locale_array[i].language))
      return language_to_locale_array[i].locale;
  }
  if (strcmp(langstr, "help") == 0 || strcmp(langstr, "list") == 0)
    show_lang_usage(0);
  fprintf(stderr, "%s is an invalid language\n", langstr);
  show_lang_usage(59);
  return NULL;
}


#if defined(__APPLE__)

/**
 * For a given locale, return the known locale that matches it closest, or exit
 * with a usage error.
 *
 * @param  inlocale       Name of some locale (e.g., "ko_US")
 * @return Known locale.  (e.g., "ko_KR.UTF-8")
 */
static const char *locale_to_closest_locale(const char *inlocale)
{
  const int numlocale = sizeof(language_to_locale_array) / sizeof(language_to_locale_array[0]);
  const char *outlocale = NULL;
  int outlocale_score = 0;
  int i = 0;
  int j = 0;

  /* find the locale with the longest string match */
  for (i = 0; i < numlocale; i++)
  {
    const char *candidate = language_to_locale_array[i].locale;

    for (j = 0; j < (int)strlen(inlocale) && j < (int)strlen(candidate); j++)
    {
      if (inlocale[j] != candidate[j])
        break;
    }

    if (j > outlocale_score)
    {
      outlocale = candidate;
      outlocale_score = j;
    }
  }

  /* locale must match at least two characters */
  if (outlocale_score < 2)
  {
    outlocale = "";
  }

  return outlocale;
}

#endif


/**
 * Set language ("langint" global) based on a given locale;
 * will try a few ways of checking, is case-insensitive, etc.
 *
 * @param loc Locale (e.g., "pt_BR.UTF-8", "pt_BR", "pt_br", etc.)
 */
static void set_langint_from_locale_string(const char *restrict loc)
{
  char *baseloc = strdup(loc);
  char *dot = strchr(baseloc, '.');
  char *at = strchr(baseloc, '@');
  char *cntrycode = strchr(baseloc, '_');
  char straux[255];
  char *ataux = NULL;
  char *ccodeaux = NULL;
  size_t len_baseloc;
  int found = 0;
  int i;

  //  printf("langint %i\n", langint);

  if (!loc)
    return;

  /* Remove the .UTF-8 extension, then
     try to find the full locale including country code and variant,
     if it fails, then try to find the language code plus the variant,
     if it still fails, try to find language and country code without the variant,
     finally scan just the lang part.
     as a last resource reverse the scanning
   */

  if (dot)
    *dot = '\0';

  if (cntrycode)
  {
    ccodeaux = strdup(cntrycode);
    *cntrycode = '\0';
  }

  if (at)
  {
    ataux = strdup(at);
    *at = '\0';

    if (cntrycode)
    {
      /* ll_CC@variant */
      //if (found == 0)  printf("ll_CC@variant check\n");
      snprintf(straux, 255, "%s%s%s", baseloc, ccodeaux, ataux);
      len_baseloc = strlen(straux);
      for (i = 0; i < NUM_LANGS && found == 0; i++)
      {
        // Case-insensitive (both "pt_BR" and "pt_br" work, etc.)
        if (len_baseloc == strlen(lang_prefixes[i]) && !strncasecmp(straux, lang_prefixes[i], len_baseloc))
        {
          langint = i;
          found = 1;
        }
      }
    }

    /* ll@variant */
    //if (found == 0)  printf("ll@variant check\n");
    snprintf(straux, 255, "%s%s", baseloc, ataux);
    len_baseloc = strlen(straux);
    for (i = 0; i < NUM_LANGS && found == 0; i++)
    {
      // Case-insensitive (both "pt_BR" and "pt_br" work, etc.)
      if (len_baseloc == strlen(lang_prefixes[i]) && !strncasecmp(straux, lang_prefixes[i], len_baseloc))
      {
        langint = i;
        found = 1;
      }
    }
  }

  if (cntrycode)
  {
    /* ll_CC */
    //if (found == 0)  printf("ll_CC check\n");
    snprintf(straux, 255, "%s%s", baseloc, ccodeaux);
    len_baseloc = strlen(straux);

    /* Which, if any, of the locales is it? */

    for (i = 0; i < NUM_LANGS && found == 0; i++)
    {
      // Case-insensitive (both "pt_BR" and "pt_br" work, etc.)
      if (len_baseloc == strlen(lang_prefixes[i]) && !strncasecmp(straux, lang_prefixes[i], strlen(lang_prefixes[i])))
      {
        langint = i;
        found = 1;
      }
    }
  }

  /* ll */
  //  if (found == 0)  printf("ll check\n");
  len_baseloc = strlen(baseloc);
  /* Which, if any, of the locales is it? */

  for (i = 0; i < NUM_LANGS && found == 0; i++)
  {
    // Case-insensitive (both "pt_BR" and "pt_br" work, etc.)
    if (len_baseloc == strlen(lang_prefixes[i]) && !strncasecmp(baseloc, lang_prefixes[i], strlen(lang_prefixes[i])))
    {
      langint = i;
      found = 1;
    }
  }

  /* Last resort, we should never arrive here, this check depends
     on the right order in lang_prefixes[] 
     Languages sharing the same starting letters must be ordered 
     from longest to shortest, like currently are pt_BR and pt */
// if (found == 0)
  // printf("Language still not found: loc= %s  Trying reverse check as last resource...\n", loc);

  for (i = 0; i < NUM_LANGS && found == 0; i++)
  {
    // Case-insensitive (both "pt_BR" and "pt_br" work, etc.)
    if (!strncasecmp(loc, lang_prefixes[i], strlen(lang_prefixes[i])))
    {
      langint = i;
      found = 1;
    }
  }
  //  printf("langint %i, lang_ext %s\n", langint, lang_prefixes[langint]);

  free(baseloc);
  if (ataux)
    free(ataux);
  if (ccodeaux)
    free(ccodeaux);
}

#define HAVE_SETENV
#ifdef WIN32
#undef HAVE_SETENV
#endif


/**
 * Set an environment variable.
 * (Wrapper for setenv() or putenv(), depending on OS)
 *
 * @param name Variable to set
 * @param value Value to set the variable to
 */
void mysetenv(const char *name, const char *value)
{
#ifndef HAVE_SETENV
  int len;
  char *str;
#endif

  if (name != NULL && value != NULL)
  {
#ifdef HAVE_SETENV
    setenv(name, value, 1);
#else
    len = strlen(name) + 1 + strlen(value) + 1;
    str = malloc(len);

    sprintf(str, "%s=%s", name, value);
    putenv(str);
#endif
  }
  else
  {
    fprintf(stderr,
            "WARNING: mysetenv() received a null pointer. name=%s, value=%s\n",
            (name == NULL ? "NULL" : name), (value == NULL ? "NULL" : value));
  }
}


/**
 * Attempt to set Tux Paint's UI language.
 *
 * @param loc Locale
 * @return The Y-nudge value for font rendering in the language.
 */

static void set_current_language(const char *restrict loc, int *ptr_num_wished_langs)
{
  int j = 0;
  char *oldloc;
  char *env_language;
  char *env_language_lang;
  char *env;
  int num_wished_langs = 0;

  *ptr_num_wished_langs = 0;

  if (strlen(loc) > 0)
  {
    /* Got command line or config file language */
    DEBUG_PRINTF("Language via config: %s\n", loc);
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "Setting LANGUAGE env var to: '%s'", loc);
#endif
    mysetenv("LANGUAGE", loc);
  }
  else
  {
    DEBUG_PRINTF("Language NOT set via config\n");
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "Language NOT set via config - using system defaults");
#endif

    /* Find what language to use from env vars */
    env = getenv("LANGUAGE");
    if (env == NULL || env[0] == '\0')
    {
      env = getenv("LC_ALL");
      if (env != NULL && env[0] != '\0')
      {
        DEBUG_PRINTF("Language via LC_ALL: %s\n", getenv("LC_ALL"));
        mysetenv("LANGUAGE", getenv("LC_ALL"));
      }
      else
      {
        env = getenv("LC_MESSAGES");
        if (env != NULL && env[0] != '\0')
        {
          DEBUG_PRINTF("Language via LC_MESSAGES: %s\n", getenv("LC_MESSAGES"));
          mysetenv("LANGUAGE", getenv("LC_MESSAGES"));
        }
        else
        {
          env = getenv("LANG");
          if (env != NULL && env[0] != '\0')
          {
            DEBUG_PRINTF("Language via LANG: %s\n", getenv("LANG"));
            mysetenv("LANGUAGE", getenv("LANG"));
          }
          else
          {
            DEBUG_PRINTF("No language set!\n");
          }
        }
      }
    }
    else
    {
      DEBUG_PRINTF("Language was set to '%s'\n", getenv("LANGUAGE"));
    }
  }

  oldloc = strdup(loc);

  /* First set the locale according to the environment, then try to overwrite with loc,
     after that, ctype_utf8() call will test the compatibility with utf8 and try to load
     a different locale if the resulting one is not compatible. */
  DEBUG_PRINTF("Locale BEFORE is: %s\n", setlocale(LC_ALL, NULL));      //EP

  setlocale(LC_ALL, "");
  setlocale(LC_ALL, loc);
  ctype_utf8();

  DEBUG_PRINTF("Locale AFTER is: %s\n", setlocale(LC_ALL, NULL));       //EP
#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "Locale AFTER setlocale(): '%s'", setlocale(LC_ALL, NULL));
#endif

#ifdef BDIST_WIN32
  // FIXME: After the update of MinGW/MSYS2 in January 2022, gettext() no longer find
  //        translation (.mo) files unless dirname is specified by full path.
  //
  //                      -- 2022/02/02: Shin-ichi TOYAMA & Pere Pujal i Carabantes
  char curdir[256];
  char f[512];

  getcwd(curdir, sizeof(curdir));
  snprintf(f, sizeof(f), "%s%s", curdir, "\\locale");
#ifdef DEBUG
  printf("Current directory at launchtime: %s\n", curdir);
  printf("Localedir is set to: %s\n", f);
#endif
  bindtextdomain("tuxpaint", f);
#else
  bindtextdomain("tuxpaint", LOCALEDIR);
#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "bindtextdomain set to: '%s'", LOCALEDIR);
#endif
#endif

  /* Old version of glibc does not have bind_textdomain_codeset() */
#if defined(_WIN32) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2) || __GLIBC__ > 2 || defined(__NetBSD__) || __APPLE__
  bind_textdomain_codeset("tuxpaint", "UTF-8");
#endif
  textdomain("tuxpaint");
#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "textdomain set to 'tuxpaint'");
#endif

  // NULL: Used to direct setlocale() to query the current
  // internationalised environment and return the name of the locale().
  loc = setlocale(LC_MESSAGES, NULL);

#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "LC_MESSAGES locale: '%s', oldloc: '%s'", 
                      loc ? loc : "NULL", oldloc ? oldloc : "NULL");
#endif

  if (oldloc && loc && strcmp(oldloc, "") != 0 && strcmp(loc, oldloc) != 0)
  {
    /* System doesn't recognize that locale!  Hack, per Albert C., is to set LC_ALL to a valid UTF-8 locale, then set LANGUAGE to the locale we want to force -bjk 2010.10.05 */

    /* Albert's comments from December 2009:
       gettext() won't even bother to look up messages unless it
       is totally satisfied that you are using one of the locales that
       it ships with! Make gettext() unhappy, and it'll switch to the
       lobotomized 7-bit Linux "C" locale just to spite you.

       http://sources.redhat.com/cgi-bin/cvsweb.cgi/libc/localedata/SUPPORTED?content-type=text/x-cvsweb-markup&cvsroot=glibc

       You can confuse gettext() into mostly behaving. For example, a
       user could pick a random UTF-8 locale and change the messages.
       In that case, Tux Paint thinks it's in the other locale but the
       messages come out right. Like so: LANGUAGE=zam LC_ALL=fr_FR.UTF-8
       It doesn't work to leave LC_ALL unset, set it to "zam", set it to "C",
       or set it to random nonsense. Yeah, Tux Paint will think it's in
       a French locale, but the messages will be Zapotec nonetheless.

       Maybe it's time to give up on gettext().

       [see also: https://sourceforge.net/mailarchive/message.php?msg_name=787b0d920912222352i5ab22834x92686283b565016b%40mail.gmail.com ]
     */

    /* Unneeded here, this has yet been done as part of ctype_utf8() call before, iterating over a list of locales */
    //    setlocale(LC_ALL, "en_US.UTF-8"); /* Is it dumb to assume "en_US" is pretty close to "C" locale? */

    mysetenv("LANGUAGE", oldloc);
    set_langint_from_locale_string(oldloc);
  }
  else
  {
#ifdef _WIN32
    if (getenv("LANGUAGE") == NULL)
      mysetenv("LANGUAGE", loc);
#endif

    if (getenv("LANGUAGE") == NULL)
      mysetenv("LANGUAGE", "C");
  }
  env_language = strdup(getenv("LANGUAGE"));

  if (*env_language)
  {
    env_language_lang = strtok(env_language, ":");
    while (env_language_lang != NULL)
    {
      num_wished_langs++;
      set_langint_from_locale_string(env_language_lang);
      wished_langs[j].langint = langint;
      wished_langs[j].lang_prefix = lang_prefixes[langint];
      wished_langs[j].need_own_font = search_int_array(langint, lang_use_own_font);
      wished_langs[j].need_right_to_left = search_int_array(langint, lang_use_right_to_left);

      j++;
      env_language_lang = strtok(NULL, ":");
    }
    if (*env_language)
      free(env_language);
  }
  //    set_langint_from_locale_string(loc);


  lang_prefix = lang_prefixes[wished_langs[0].langint];

  short_lang_prefix = strdup(lang_prefix);
  /* When in doubt, cut off country code */
  if (strchr(short_lang_prefix, '_'))
    *strchr(short_lang_prefix, '_') = '\0';

  need_own_font = wished_langs[0].need_own_font;
  need_right_to_left = wished_langs[0].need_right_to_left;

#ifdef DEBUG
  fprintf(stderr, "DEBUG: Language is %s (%d) %s\n", lang_prefix, langint, need_right_to_left ? "(RTL)" : "");
  fflush(stderr);
#endif

  free(oldloc);

  DEBUG_PRINTF("lang_prefixes[%d] is \"%s\"\n", get_current_language(), lang_prefixes[get_current_language()]);

  *ptr_num_wished_langs = num_wished_langs;
}


/**
 * Given a locale (e.g., "de_DE.UTF-8" or a language name (e.g., "german"),
 * attempt to set Tux Paint's UI language.  Show help, and exit,
 * if asked (either 'locale' or 'lang' are "help"), or if the
 * given input is not recognized.
 *
 * @param char * lang Language name (or NULL)
 * @param char * locale Locale (or NULL)
 * @param int * a place to return the number of languages we want to use, when scanning stamp descriptions 
 * @return Y-nudge
 */
void setup_i18n(const char *restrict lang, const char *restrict locale, int *num_wished_langs)
{
  DEBUG_PRINTF("lang %p, locale %p\n", lang, locale);
  DEBUG_PRINTF("lang \"%s\", locale \"%s\"\n", lang, locale);

#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "setup_i18n() called: lang='%s', locale='%s'", 
                      lang ? lang : "NULL", locale ? locale : "NULL");
#endif

  if (locale)
  {
    if (!strcmp(locale, "help"))
    {
      show_locale_usage(stdout, "tuxpaint");
      exit(0);
    }
  }
  else
  {
#if defined(__APPLE__)
    locale = locale_to_closest_locale(apple_locale());
#else
    locale = "";
#endif
  }

  if (lang)
    locale = language_to_locale(lang);

#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "After language_to_locale(): locale='%s'", locale ? locale : "NULL");
  
  if (locale == NULL)
  {
    locale = android_locale();
    __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "Using android_locale(): '%s'", locale ? locale : "NULL");
  }
#else
  if (locale == NULL)
    locale = android_locale();
#endif

  if (locale == NULL)
    locale = "";

#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO, "TuxPaint", "Final locale before set_current_language(): '%s'", locale);
#endif

  set_current_language(locale, num_wished_langs);
}
