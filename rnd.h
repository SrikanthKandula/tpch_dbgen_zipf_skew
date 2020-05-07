/*
* $Id: rnd.h,v 1.4 2006/08/01 04:13:17 jms Exp $
*
* Revision History
* ===================
* $Log: rnd.h,v $
* Revision 1.4  2006/08/01 04:13:17  jms
* fix parallel generation
*
* Revision 1.3  2006/07/31 17:23:09  jms
* fix to parallelism problem
*
* Revision 1.2  2005/01/03 20:08:59  jms
* change line terminations
*
* Revision 1.1.1.1  2004/11/24 23:31:47  jms
* re-establish external server
*
* Revision 1.1.1.1  2003/08/08 21:50:34  jms
* recreation after CVS crash
*
* Revision 1.3  2003/08/08 21:35:26  jms
* first integration of rng64 for o_custkey and l_partkey
*
* Revision 1.2  2003/08/07 17:58:34  jms
* Convery RNG to 64bit space as preparation for new large scale RNG
*
* Revision 1.1.1.1  2003/04/03 18:54:21  jms
* initial checkin
*
*
*/
/*
 * rnd.h -- header file for use withthe portable random number generator
 * provided by Frank Stephens of Unisys
 */

/* function protypes */
DSS_HUGE            NextRand    PROTO((DSS_HUGE));
DSS_HUGE            UnifInt     PROTO((DSS_HUGE, DSS_HUGE, long));
DSS_HUGE            ZipfInt     PROTO((DSS_HUGE, DSS_HUGE, long, DSS_HUGE));

static long     nA = 16807;     /* the multiplier */
static long     nM = 2147483647;/* the modulus == 2^31 - 1 */
static long     nQ = 127773;    /* the quotient nM / nA */
static long     nR = 2836;      /* the remainder nM % nA */

double   dM = 2147483647.0;

/*
 * macros to control RNG and assure reproducible multi-stream
 * runs without the need for seed files. Keep track of invocations of RNG
 * and always round-up to a known per-row boundary.
 */
/* 
 * preferred solution, but not initializing correctly
 */
#define VSTR_MAX(len)	(long)(len / 5 + (len % 5 == 0)?0:1 + 1)
seed_t     Seed[MAX_STREAM + 1] =
{
    {PART,   1,          0,	10},					/* P_MFG_SD     0 */
    {PART,   46831694,   0, 10},					/* P_BRND_SD    1 */
    {PART,   1841581359, 0, 10},				/* P_TYPE_SD    2 */
    {PART,   1193163244, 0, 10},					/* P_SIZE_SD    3 */
    {PART,   727633698,  0, 10},					/* P_CNTR_SD    4 */
    {NONE,   933588178,  0, 1},					/* text pregeneration  5 */
    {PART,   804159733,  0, 10},	/* P_CMNT_SD    6 */
    {PSUPP,  1671059989, 0, SUPP_PER_PART * 10},     /* PS_QTY_SD    7 */ // double these numbers to account for changing #supp_per_part
    {PSUPP,  1051288424, 0, SUPP_PER_PART * 10},     /* PS_SCST_SD   8 */
    {PSUPP,  1961692154, 0, SUPP_PER_PART * 10},     /* PS_CMNT_SD   9 */
    {ORDER,  1227283347, 0, 1},				    /* O_SUPP_SD    10 */
    {ORDER,  1171034773, 0, 10},					/* O_CLRK_SD    11 */
    {ORDER,  276090261,  0, 10},  /* O_CMNT_SD    12 */
	{ORDER,  1066728069, 0, 10},					/* O_ODATE_SD   13 */
    {LINE,   209208115,  0, O_LCNT_MAX * 10},        /* L_QTY_SD     14 */
    {LINE,   554590007,  0, O_LCNT_MAX * 10},        /* L_DCNT_SD    15 */
    {LINE,   721958466,  0, O_LCNT_MAX * 10},        /* L_TAX_SD     16 */
    {LINE,   1371272478, 0, O_LCNT_MAX * 10},        /* L_SHIP_SD    17 */
    {LINE,   675466456,  0, O_LCNT_MAX * 10},        /* L_SMODE_SD   18 */
    {LINE,   1808217256, 0, O_LCNT_MAX * 10},      /* L_PKEY_SD    19 */
    {LINE,   2095021727, 0, O_LCNT_MAX * 10},      /* L_SKEY_SD    20 */
    {LINE,   1769349045, 0, O_LCNT_MAX * 10},      /* L_SDTE_SD    21 */
    {LINE,   904914315,  0, O_LCNT_MAX * 10},      /* L_CDTE_SD    22 */
    {LINE,   373135028,  0, O_LCNT_MAX * 10},      /* L_RDTE_SD    23 */
    {LINE,   717419739,  0, O_LCNT_MAX * 10},      /* L_RFLG_SD    24 */
    {LINE,   1095462486, 0, O_LCNT_MAX * 10},   /* L_CMNT_SD    25 */
    {CUST,   881155353,  0, 90},      /* C_ADDR_SD    26 */
    {CUST,   1489529863, 0, 1},      /* C_NTRG_SD    27 */
    {CUST,   1521138112, 0, 30},      /* C_PHNE_SD    28 */
    {CUST,   298370230,  0, 10},      /* C_ABAL_SD    29 */
    {CUST,   1140279430, 0, 10},      /* C_MSEG_SD    30 */
    {CUST,   1335826707, 0, 10},     /* C_CMNT_SD    31 */
    {SUPP,   706178559,  0, 90},      /* S_ADDR_SD    32 */
    {SUPP,   110356601,  0, 1},      /* S_NTRG_SD    33 */
    {SUPP,   884434366,  0, 30},      /* S_PHNE_SD    34 */
    {SUPP,   962338209,  0, 10},      /* S_ABAL_SD    35 */
    {SUPP,   1341315363, 0, 10},     /* S_CMNT_SD    36 */
    {PART,   709314158,  0, 92},      /* P_NAME_SD    37 */
    {ORDER,  591449447,  0, 10},      /* O_PRIO_SD    38 */
    {LINE,   431918286,  0, 1},      /* HVAR_SD      39 */
    {ORDER,  851767375,  0, 10},      /* O_CKEY_SD    40 */
    {NATION, 606179079,  0, 10},      /* N_CMNT_SD    41 */
    {REGION, 1500869201, 0, 10},      /* R_CMNT_SD    42 */
    {ORDER,  1434868289, 0, 10},      /* O_LCNT_SD    43 */
    {SUPP,   263032577,  0, 10},      /* BBB offset   44 */
    {SUPP,   753643799,  0, 10},      /* BBB type     45 */
    {SUPP,   202794285,  0, 10},      /* BBB comment  46 */
    {SUPP,   715851524,  0, 10},       /* BBB junk     47 */
    {NATION, 342333517, 0, 10},       /* N_CMNT_SD_LEN  48 */
    {REGION, 98711209432, 0, 10},     /* R_CMNT_SD_LEN  49 */
    {CUST,   343423, 0, 10},          /* C_CMNT_SD_LEN 50 */
    {ORDER,  65661284343, 0, 10},     /* O_CMNT_SD_LEN 51 */
    {LINE,   234307599217, 0, O_LCNT_MAX * 10},    /* L_CMNT_SD_LEN 52 */
    {PART,   9001573244, 0, 10},      /* P_CMNT_SD_LEN 53 */
    {PSUPP,  67734243431, 0, SUPP_PER_PART * 10},     /* PS_CMNT_SD_LEN 54 */
    {SUPP,   887643092347, 0, 10},    /* S_CMNT_SD_LEN 55 */
 //   {PSUPP, 340256915, 0, 1} /* PartSupp; vary number of suppliers per part 48 SUPP_PER_PART_SD */
};
