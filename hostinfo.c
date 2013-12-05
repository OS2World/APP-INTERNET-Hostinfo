/* hostinfo.c: a simple wrapper around gethostbyname() and gethostbyaddr().
 * created 1998-Apr-17 jmk (after foolishly deleting it)
 * autodate: 2000-Jul-20 07:23
 *
 * by Jim Knoble <jmknoble@jmknoble.cx>
 * Copyright (C) 1998,1999,2000 Jim Knoble
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * express or implied, including but not limited to the warranties of
 * merchantability, fitness for a particular purpose and
 * noninfringement. In no event shall the author(s) be liable for any
 * claim, damages or other liability, whether in an action of
 * contract, tort or otherwise, arising from, out of or in connection
 * with the software or the use or other dealings in the software.
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation.
 */

#include <netdb.h>
#include <stdio.h>
#include <string.h>

/* This is arbitrarily dependent on IPv4 to be right. */
#define ADDR_LEN	(4)

/* AF_INET doesn't appear to be defined in the Linux libc5 includes,
 * although it's mentioned in the man pages.  I determined this value
 * empirically.
 */
#ifndef AF_INET
#define AF_INET		(0x2)
#endif /* AF_INET */

static char *prog_name;
static int output_base = 10;
static int input_base = 10;
static int force_name = 0;
static int show_all = 1;
static int show_name = 0;
static int show_addresses = 0;
static int show_aliases = 0;
static int show_first_only = 0;
static int print_only = 0;

static char *usage_info =
"Usage:\n"
"  %s [-a] [-n] [-l] [-1] [-{x|o|d}] [-{X|O|D|N}] [-p] <host> [<host>...]\n"
"  %s --help\n"
"  %s --version\n"
"\n"
"Find and print name, IP address(es), and aliases (if any) of each\n"
"specified host.  By default, first try to interpret <host> as a\n"
"numeric dotted-quad IPv4 address.  If <host> doesn't appear to be a\n"
"numeric IP address, assume it is a hostname to look up.  The -N option\n"
"forces <host> to be interpreted as a hostname.\n"
"\n"
"Host information comes from the gethostbyname(3) and/or\n"
"gethostbyaddr(3) function calls.  These function calls use some\n"
"combination of both the \"hosts database\" (usually, '/etc/hosts'\n"
"and/or NIS/NIS+), and, if available, the system's DNS resolver.  What\n"
"combination and in what order is configured in an operating-system\n"
"dependent manner (and is outside the scope of this program).\n"
"\n"
"If none of -a, -n, or -l are given, host information prints out in a\n"
"well-labelled, human-readable format.  For example:\n"
"\n"
"   address: 127.0.0.1\n"
"  hostname: localhost\n"
"   aliases: localhost.localdomain\n"
"\n"
"If you use one or more of the -a, -n, or -l options, host information\n"
"prints in a more compact format, with requested fields separated by\n"
"tab characters.  For example:\n"
"\n"
"  127.0.0.1\tlocalhost\tlocalhost.localdomain\n"
"\n"
"Note that some hostnames map to multiple IP addresses.  By default,\n"
NAME " prints a separate record for each address.  Use the -1 option\n"
"to cause only the first address to be printed.\n"
"\n"
"The -p option causes " NAME " to interpret each host as an IP address,\n"
"skip the lookup stage, and print the address in the requested format.\n"
"Together with the -x/-o/-d and -X/-O/-D options, this turns " NAME "\n"
"into a quick tool for converting between IP address formats.\n"
"\n"
"Options:\n"
"  -a, --address             Print only the IP address(es) of each host.\n"
"\n"
"  -n, --name                Print only the name of each host.\n"
"\n"
"  -l, --aliases             Print only the aliases, if any, for each host.\n"
"\n"
"  -1, --first-address       If a host has multiple IP addresses, only\n"
"                            display the first one.\n"
"\n"
"  -x, --print-hexadecimal   Print IP addresses in hexadecimal (base 16).\n"
"\n"
"  -o, --print-octal         Print IP addresses in octal (base 8).\n"
"\n"
"  -d, --print-decimal       Print IP addresses in decimal (base 10).  This\n"
"                            is the default.\n"
"\n"
"  -X, --expect-hexadecimal  Try to interpret <host> as a hexadecimal (base\n"
"                            16) IP address, in the format 'wwxxyyzz'\n"
"                            (e.g., 7f000001).\n"
"\n"
"  -O, --expect-octal        Try to interpret <host> as an octal (base 8)\n"
"                            IP address, in the format '\\qqq\\rrr\\sss\\ttt'\n"
"                            (e.g., \\177\\000\\000\\001).\n"
"\n"
"  -D, --expect-decimal      Try to interpret <host> as a decimal (base 10)\n"
"                            IP address, in the normal dotted-quad format\n"
"                            'nnn.mmm.ppp.rrr' (e.g., 127.0.0.1).  This is\n"
"                            the default.\n"
"\n"
"  -N, --force-name          Don't try to interpret <host> as an IP address;\n"
"                            assume each specified host is a hostname.\n"
"\n"
"  -p, --print               Don't try to look up <host>; if it's a valid\n"
"                            IP address, print it in the requested format\n"
"                            (decimal [default], hexadecimal, or octal).\n"
"\n"
"  -h, --help                Display this builtin usage information.\n"
"\n"
"  -V, --version             Display program version, copyright, and\n"
"                            warranty information.\n"
;

static char *copyright_info =
"by Jim Knoble <jmknoble@jmknoble.cx>\n"
"Copyright (C) 1998,1999,2000 Jim Knoble\n"
"\n"
"THIS SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
"express or implied, including but not limited to the warranties of\n"
"merchantability, fitness for a particular purpose and\n"
"noninfringement. In no event shall the author(s) be liable for any\n"
"claim, damages or other liability, whether in an action of\n"
"contract, tort or otherwise, arising from, out of or in connection\n"
"with the software or the use or other dealings in the software.\n"
"\n"
"Permission to use, copy, modify, distribute, and sell this software\n"
"and its documentation for any purpose is hereby granted without\n"
"fee, provided that the above copyright notice appear in all copies\n"
"and that both that copyright notice and this permission notice\n"
"appear in supporting documentation.\n"
;

void print_help() {
   printf(usage_info, prog_name, prog_name, prog_name);
   exit(1);
}

void print_version() {
   printf("%s version %s (%s)\n", NAME, VERSION, DATE);
   printf(copyright_info);
   exit(2);
}

int check_conflict2(char **argv, int argi1, int argi2) {
   int conflict = 0;
   if ((argi1 >= 0) && (argi2 >= 0)) {
      conflict++;
      fprintf(stderr,
	      "error: Options '%s' and '%s' don't make sense together.\n",
	      argv[argi1], argv[argi2]);
   }
   return(conflict);
}

int check_conflict3_nonrecursive(char **argv, int argi1, int argi2,
				 int argi3) {
   int conflict = 0;
   if ((argi1 >= 0) && (argi2 >= 0) && (argi3 >= 0)) {
      conflict++;
      fprintf(stderr,
	      "error: Options '%s', '%s', and '%s' don't make sense "
	      "together.\n",
	      argv[argi1], argv[argi2], argv[argi3]);
   }
   return(conflict);
}

int check_conflict3(char **argv, int argi1, int argi2, int argi3) {
   int conflict = 0;
   conflict = check_conflict3_nonrecursive(argv, argi1, argi2, argi3);
   if (!conflict) {
      conflict += check_conflict2(argv, argi1, argi2);
      conflict += check_conflict2(argv, argi1, argi3);
      conflict += check_conflict2(argv, argi2, argi3);
   }
   return(conflict);
}

int check_conflict4(char **argv, int argi1, int argi2, int argi3, int argi4) {
   int conflict = 0;
   if ((argi1 >= 0) && (argi2 >= 0) && (argi3 >= 0) && (argi4 >= 0)) {
      conflict++;
      fprintf(stderr,
	      "error: Options '%s', '%s', '%s', and '%s' don't make sense "
	      "together.\n",
	      argv[argi1], argv[argi2], argv[argi3], argv[argi4]);
   } else {
      conflict += check_conflict3_nonrecursive(argv, argi1, argi2, argi3);
      conflict += check_conflict3_nonrecursive(argv, argi1, argi2, argi4);
      conflict += check_conflict3_nonrecursive(argv, argi1, argi3, argi4);
      conflict += check_conflict3_nonrecursive(argv, argi2, argi3, argi4);
      if (!conflict) {
	 conflict += check_conflict2(argv, argi1, argi2);
	 conflict += check_conflict2(argv, argi1, argi3);
	 conflict += check_conflict2(argv, argi1, argi4);
	 conflict += check_conflict2(argv, argi2, argi3);
	 conflict += check_conflict2(argv, argi2, argi4);
	 conflict += check_conflict2(argv, argi3, argi4);
      }
   }
   return(conflict);
}

int process_arguments(int argc, char **argv) {
   int i;
   int ibase16_arg, ibase10_arg, ibase08_arg;
   int obase16_arg, obase10_arg, obase08_arg;
   int force_name_arg, print_only_arg;
   int syntax_error = 0;
   
   ibase16_arg = ibase10_arg = ibase08_arg = -1;
   obase16_arg = obase10_arg = obase08_arg = -1;
   force_name_arg = print_only_arg = -1;
   
   for (i = 1; i < argc; i++) {
      if ((0 == strcmp(argv[i], "-h")) ||
	  (0 == strcmp(argv[i], "-help")) ||
	  (0 == strcmp(argv[i], "--help")) ||
	  (0 == strcmp(argv[i], "-?"))) {
	 print_help();
      } else if ((0 == strcmp(argv[i], "-V")) ||
		 (0 == strcmp(argv[i], "--version"))) {
	 print_version();
      } else if ((0 == strcmp(argv[i], "-a")) ||
		 (0 == strcmp(argv[i], "--address")) ||
		 (0 == strcmp(argv[i], "--addresses"))) {
	 show_all = 0;
	 show_addresses = 1;
      } else if ((0 == strcmp(argv[i], "-n")) ||
		 (0 == strcmp(argv[i], "--name")) ||
		 (0 == strcmp(argv[i], "--hostname"))) {
	 show_all = 0;
	 show_name = 1;
      } else if ((0 == strcmp(argv[i], "-l")) ||
		 (0 == strcmp(argv[i], "--alias")) ||
		 (0 == strcmp(argv[i], "--aliases"))) {
	 show_all = 0;
	 show_aliases = 1;
      } else if ((0 == strcmp(argv[i], "-1")) ||
		 (0 == strcmp(argv[i], "--first")) ||
		 (0 == strcmp(argv[i], "--first-address"))) {
	 show_first_only = 1;
      } else if ((0 == strcmp(argv[i], "-x")) ||
		 (0 == strcmp(argv[i], "--ox")) ||
		 (0 == strcmp(argv[i], "--hex")) ||
		 (0 == strcmp(argv[i], "--hexadecimal")) ||
		 (0 == strcmp(argv[i], "--print-hex")) ||
		 (0 == strcmp(argv[i], "--print-hexadecimal"))) {
	 output_base = 16;
	 obase16_arg = i;
      } else if ((0 == strcmp(argv[i], "-o")) ||
		 (0 == strcmp(argv[i], "--oo")) ||
		 (0 == strcmp(argv[i], "--oct")) ||
		 (0 == strcmp(argv[i], "--octal")) ||
		 (0 == strcmp(argv[i], "--print-oct")) ||
		 (0 == strcmp(argv[i], "--print-octal"))) {
	 output_base = 8;
	 obase08_arg = i;
      } else if ((0 == strcmp(argv[i], "-d")) ||
		 (0 == strcmp(argv[i], "--od")) ||
		 (0 == strcmp(argv[i], "--dec")) ||
		 (0 == strcmp(argv[i], "--decimal")) ||
		 (0 == strcmp(argv[i], "--print-dec")) ||
		 (0 == strcmp(argv[i], "--print-decimal"))) {
	 output_base = 10;
	 obase10_arg = i;
      } else if ((0 == strcmp(argv[i], "-X")) ||
		 (0 == strcmp(argv[i], "--ix")) ||
		 (0 == strcmp(argv[i], "--expect-hex")) ||
		 (0 == strcmp(argv[i], "--expect-hexadecimal"))) {
	 input_base = 16;
	 ibase16_arg = i;
      } else if ((0 == strcmp(argv[i], "-O")) ||
		 (0 == strcmp(argv[i], "--io")) ||
		 (0 == strcmp(argv[i], "--expect-oct")) ||
		 (0 == strcmp(argv[i], "--expect-octal"))) {
	 input_base = 8;
	 ibase08_arg = i;
      } else if ((0 == strcmp(argv[i], "-D")) ||
		 (0 == strcmp(argv[i], "--id")) ||
		 (0 == strcmp(argv[i], "--expect-dec")) ||
		 (0 == strcmp(argv[i], "--expect-decimal"))) {
	 input_base = 10;
	 ibase10_arg = i;
      } else if ((0 == strcmp(argv[i], "-N")) ||
		 (0 == strcmp(argv[i], "--isname")) ||
		 (0 == strcmp(argv[i], "--force-name"))) {
	 force_name = 1;
	 force_name_arg = i;
      } else if ((0 == strcmp(argv[i], "-p")) ||
		 (0 == strcmp(argv[i], "--print"))) {
	 print_only = 1;
	 print_only_arg = i;
      } else if (0 == strcmp(argv[i], "--")) {
	 i++;
	 break;
      } else if ('-' == argv[i][0]) {
	 fprintf(stderr, "error: I don't understand option '%s'.\n", argv[i]);
	 syntax_error = 1;
      } else {
	 break;
      }
   }
   if (i >= argc) {
      fprintf(stderr, "error: Please specify a hostname or host address to "
	      "look up.\n");
      syntax_error++;
   }
   syntax_error += check_conflict3(argv, obase16_arg, obase08_arg,
				   obase10_arg);
   syntax_error += check_conflict4(argv, ibase16_arg, ibase08_arg,
				   ibase10_arg, force_name_arg);
   syntax_error += check_conflict2(argv, force_name_arg, print_only_arg);
   if (syntax_error) {
      fprintf(stderr, "Use '%s --help' for usage information.\n", prog_name);
      exit(1);
   }
   return(i);
}

int main(int argc, char **argv) {
   int arg;
   int start_arg;
   
   char *iformat;
   char *init_oformat;
   char *dot_oformat;
   
   char *host_str;
   char host_addr[ADDR_LEN];
   unsigned a[ADDR_LEN];
   int n;
   
   struct hostent *h;
   struct hostent dummy_hostent;
   char *dummy_aliases[1];
   char *dummy_addr_list[2];

   int i;
   int b;
   int alias;

   int status = 0;
   int get_by_addr = 0;
   
   if (argc < 1) {
      prog_name = NAME;
   } else {
     prog_name = argv[0];
   }
   
   start_arg = process_arguments(argc, argv);

   /* One-time post-option stuff */
   if (show_all) {
      show_name = show_addresses = show_aliases = 1;
   }
   if (16 == input_base) {
      iformat = "%2x%2x%2x%2x";
   } else if (8 == input_base) {
      iformat = "\\%3o\\%3o\\%3o\\%3o";
   } else {
      iformat = "%u.%u.%u.%u";
   }
   if (16 == output_base) {
      init_oformat = "%02x";
      dot_oformat = init_oformat;
   } else if (8 == output_base) {
      init_oformat = "\\%03o";
      dot_oformat = init_oformat;
   } else {
      init_oformat = "%d";
      dot_oformat = ".%d";
   }
   
   for (arg = start_arg; arg < argc; arg++) {
      host_str = argv[arg];
      
      /* Try to convert host_str to an IP address. */
      if (print_only || (!force_name)) {
	 n = sscanf(host_str, iformat, &(a[0]), &(a[1]), &(a[2]), &(a[3]));
	 if (ADDR_LEN == n) {
	    if ((a[0] > 255) || (a[1] > 255) || (a[2] > 255) || (a[3] > 255)) {
	       printf("%s: Invalid host address\n", host_str);
	       status = 1;
	       break;
	    } else {
	       get_by_addr = 1;
	       for (i = 0; i < ADDR_LEN; i++) {
		  host_addr[i] = (char) a[i];
	       }
	    }
	 }
      }
      
      if (print_only) {
	 h = &dummy_hostent;
	 h->h_name = "";
	 h->h_aliases = dummy_aliases;
	 h->h_aliases[0] = NULL;
	 h->h_addrtype = AF_INET;
	 h->h_length = ADDR_LEN;
	 h->h_addr_list = dummy_addr_list;
	 h->h_addr_list[0] = host_addr;
	 h->h_addr_list[1] = NULL;
      } else {
	 /* If we got an IP address, use that to get host info;
	  * otherwise, assume it's a hostname.
	  */
	 if (get_by_addr) {
	    h = gethostbyaddr(host_addr, ADDR_LEN, AF_INET);
	 } else {
	    h = gethostbyname(host_str);
	 }
	 if (NULL == h) {
	    herror(host_str);
	    status = 10;
	    break;
	 }
      }
      
      /* Print one record for each address in the resulting
       * hostent structure. */
      for (i = 0; NULL != h->h_addr_list[i]; i++) {
#ifdef DEBUG
	 if (show_all) {
	    printf("\n addrtype: %#x\n", h->h_addrtype);
	 }
#endif
	 if (show_addresses) {
	    if (show_all) {
	       printf(" address: ");
	    }
	    printf(init_oformat, (unsigned char) h->h_addr_list[i][0]);
	    for (b = 1; b < h->h_length; b++) {
	       printf(dot_oformat, (unsigned char) h->h_addr_list[i][b]);
	    }
	    if (show_all) {
	       printf("\n");
	    }
	 }
	 if (show_name) {
	    if (show_all) {
	       printf("hostname: ");
	    } else if (show_addresses) {
	       printf("\t");
	    }
	    printf("%s", h->h_name);
	    if (show_all) {
	       printf("\n");
	    }
	 }
	 if (show_aliases) {
	    if (show_all) {
	       printf(" aliases: ");
	    } else if (show_addresses || show_name) {
	       printf("\t");
	    }
	    for (alias = 0; NULL != h->h_aliases[alias]; alias++) {
	       if (alias > 0) {
		  printf(" ");
	       }
	       printf("%s", h->h_aliases[alias]);
	    }
	 }
	 printf("\n");
	 if (show_all) {
	    printf("\n");
	 }
	 if (show_first_only) {
	    break;
	 }
      }
   }
   return(status);
}
