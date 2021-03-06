{ "L", OPT_EXIT, {(void*)show_license}, "show license" },
{ "h", OPT_EXIT, {(void*)show_help}, "show help" },
{ "?", OPT_EXIT, {(void*)show_help}, "show help" },
{ "help", OPT_EXIT, {(void*)show_help}, "show help" },
{ "-help", OPT_EXIT, {(void*)show_help}, "show help" },
{ "version", OPT_EXIT, {(void*)show_version}, "show version" },
{ "formats"  , OPT_EXIT, {(void*)show_formats  }, "show available formats" },
{ "codecs"   , OPT_EXIT, {(void*)show_codecs   }, "show available codecs" },
{ "bsfs"     , OPT_EXIT, {(void*)show_bsfs     }, "show available bit stream filters" },
{ "protocols", OPT_EXIT, {(void*)show_protocols}, "show available protocols" },
{ "filters",   OPT_EXIT, {(void*)show_filters  }, "show available filters" },
{ "pix_fmts" , OPT_EXIT, {(void*)show_pix_fmts }, "show available pixel formats" },
{ "loglevel", HAS_ARG, {(void*)opt_loglevel}, "set libav* logging level", "loglevel" },
