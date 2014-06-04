#include "stdafx.h"
#include "sysinv.h"
#include "argparser.h"

#define APP_NAME	"sysinv"
#define OUT_XML		0x1
#define OUT_JSON	0x2
#define OUT_LIST	0x3

void print_usage(int ret);

int main(int argc, CHAR* argv[])
{
	FILE *out = stdout;
	PNODE root, software, hardware, storage, configuration, node;
	DWORD format = OUT_XML;
	DWORD i = 0;
	PARGLIST argList = parse_args(argc, argv);
	PARG arg;

	for(i = 0; i < argList->count; i++) {
		arg = &argList->args[i];

		// Parse help request
		if(0 == strcmp("/?", arg->arg) || 0 == strcmp("-?", arg->arg)
			|| 0 == stricmp("/h", arg->arg) || 0 == strcmp("-h", arg->arg)
			|| 0 == strcmp("--help", arg->arg)) {
			print_usage(0);
		}

		// Parse file output argument
		else if(0 == stricmp("/f", arg->arg) || 0 == strcmp("-f", arg->arg)) {
			if(NULL == arg->val) {
				fprintf(stderr, "File name not specified.\n");
				exit(1);
			}

			if(NULL == (out = fopen(arg->val, "w"))) {
				fprintf(stderr, "Unable to open '%s' for writing.\n", arg->val);
				exit(1);
			}
		}

		// Parse output arguments
		else if(0 == stricmp("/o", arg->arg) || 0 == strcmp("-o", arg->arg)) {
			if(NULL == arg->val) {
				fprintf(stderr, "Output format not specified.\n");
				exit(1);
			}

			if(0 == stricmp("xml", arg->val))
				format = OUT_XML;

			else if(0 == stricmp("json", arg->val))
				format = OUT_JSON;

			else if(0 == stricmp("list", arg->val))
				format = OUT_LIST;

			else {
				fprintf(stderr, "Unknown output type: '%s'\n", arg->val);
				exit(1);
			}
		}
	}
	
	free(argList);

	// Build info nodes
	root = GetSystemNode();

	software = node_append_new(root, L"Software", NODE_FLAG_PLACEHOLDER);
	hardware = node_append_new(root, L"Hardware", NODE_FLAG_PLACEHOLDER);
	configuration = node_append_new(root, L"Configuration", NODE_FLAG_PLACEHOLDER);
	storage = node_append_new(configuration, L"Storage", NODE_FLAG_PLACEHOLDER);

	// Get OS info
	node = GetOperatingSystemNode();
	node_append_child(software, node);

	// Get Software packages
	node = GetPackagesNode();
	node_append_child(software, node);

	// Get CPU info
	node = GetProcessorsNode();
	node_append_child(hardware, node);

	// get volume info
	node = GetVolumesNode();
	node_append_child(storage, node);
	
	// Get disks
	node = GetDisksNode();
	node_append_child(hardware, node);

	// Print
	switch(format) {
	case OUT_XML:
		node_to_xml(root, out, NODE_XML_FLAG_NOATTS);
		break;

	case OUT_JSON:
		node_to_json(root, out, 0);
		break;

	case OUT_LIST:
		node_to_list(root, out, 0);
		break;
	}

	fclose(out);
	node_free(root, true);

	return 0;
}

void print_usage(int ret)
{
	printf("Display system information for the current host in parsable formats.\n\n");
	printf("%s [/F:filename] [/O:format]\n\n", APP_NAME);
	printf("  /F          Write to file instead of printing to screen\n");
	printf("  /O          Change output format\n");
	printf("                XML   Output data as XML tree\n");
	printf("                JSON  Output data as Javascript object\n");
	printf("                LIST  Output data as snmp-walk style list\n");
	printf("\n");
	exit(ret);
}