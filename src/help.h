#ifndef  __swaystatus_help_H__
# define __swaystatus_help_H__

static const char * const help = 
    "Usage: swaystatus [options] configuration_filename\n\n"
    "  --help                  Show help message and exit\n"
    "  --time-format=format    Overrides the default time format.\n"
    "                          The time format is specified in man page of strftime.\n"
    "Config file format:\n\n"
    "    {\n"
    "        \"name\": {\n"
    "            \"format\": \"...\",\n"
    "            \"color\": \"...\",\n"
    "            \"background: \"...\",\n"
    "            \"border\": \"...\",\n"
    "            \"border_top\": 1,\n"
    "            \"border_bottom\": 1,\n"
    "            \"border_left\": 1,\n"
    "            \"border_right\": 1,\n"
    "            \"min_width\": 1,\n"
    "            \"align\": \"...\",\n"
    "            \"urgent\": true,\n"
    "            \"separator\": true,\n"
    "            \"separator_block_width\": 9\n"
    "        },\n"
    "    }\n\n"
    "All property present for \"name\" above are optional.\n"
    "NOTE that property \"format\" is now only supported by time.\n\n"
    "The following values are valid name:\n\n"
    " - brightness\n"
    " - volume\n"
    " - battery\n"
    " - network_interface\n"
    " - load\n"
    " - memory_usage\n"
    " - time\n\n"
    "If you want to disable a certain feature, say brightness,\n"
    "then add the following to your configuration:\n\n"
    "    {\n"
    "        \"brightness\": false,\n"
    "    }";

#endif
