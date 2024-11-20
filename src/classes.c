#include <windows.h>
#include <stdio.h>

#include "utils.h"

#define VERSION "2024-11-18"

typedef struct Program_options
{
	bool print_to_screen;
	bool debug_output;
} Program_options;

typedef struct Report_summary
{
	u32 num_classes;
	u32 num_pages;
} Report_summary;

typedef struct Class
{
	char class_id[8]; // 4 + \0 (not in this report)
	char description[32]; // 30 + \0
	i32  history_periods;
	char history_by_class;
} Class;

void ParseClasses(char* data, FILE* output_file, Program_options options, Report_summary* summary)
{
	size_t index			= 0;
	size_t line_start_index = 0;
	size_t line_position	= 0;
	i32 page_header_line	= 8;

	char history_period_text[4] = {0};
	Class class = {0};
	Class class_reset = {0};

	if (output_file && !options.debug_output)
	{
		fprintf(output_file, "Class|Description\n");
	}

	while (data[index])
	{
		if (data[index] != '\n')
		{
			line_position++;
			index++;
		}
		else
		{
			if (line_position == 0)
			{
				// loop over header lines
				while (data[index] && page_header_line > 0) // data[index] != '\0' is necessary in the case the report ends with a header.
				{
					if (data[index] == '\n')
					{
						page_header_line--;
					}
					index++;
				}
				page_header_line = 8;
				summary->num_pages++;
				continue;
			}

			line_start_index = index - line_position;

			if (data[index - 1] == '-') // @HACK: Reached the end of the report
			{
				break;
			}
			size_t class_char_length = FillTextFieldAndTrim(class.class_id, &data[line_start_index + 18], 4);
			if (class_char_length != 1) // (zero characters + \0)
			{
				FillTextFieldAndTrim(class.description, &data[line_start_index + 25], 32);
				FillTextFieldAndTrim(history_period_text, &data[line_start_index + 57], 2);
				class.history_periods = atoi(history_period_text);
				class.history_by_class = data[index - 1];
			}
			else // Must be a class 'header'.
			{
				class = class_reset;
				line_position = 0;
				index++;
				continue;
			}

			char buffer[256] = {0};
			if (options.debug_output)
			{
				sprintf_s(buffer, sizeof(buffer),
				   	"                  %-4s   %-30s  %-2d              %c\n",
				   	class.class_id,
				   	class.description,
				   	class.history_periods,
				   	class.history_by_class
				   	);
			}
			else
			{
				sprintf_s(buffer, sizeof(buffer), "%s|%s\n", class.class_id, class.description);
			}

			if (output_file)
			{
				fprintf(output_file, "%s", buffer);
			}
			if (options.print_to_screen)
			{
				printf("%s", buffer);
			}

			class = class_reset;
			summary->num_classes++;

			line_position = 0;
			index++;
		}
	}
}

#define USAGE_STRING "\
%s %s\n\
John Hosick <john@atikokancastle.com>\n\n\
%s is used to process a ProfitMaster IRK class report and output a\n\
pipe-delimited file for use in the conversion to CashierPRO.\n\n\
USAGE: %s [OPTIONS] <inputfile> [outputfile]\n\
  OPTIONS:\n\
    -d, --debug     Dump output in original format (to check the correctness of the parse).\n\
    -h, --help      Show this help message.\n\
    -p, --print     Print to the screen. Note: This option does not preclude\n\
                    outputting to a file: If an output file is specified,\n\
                    it will be created as well as displayed on the screen.\n"

void PrintUsageAndExit(char *program_name)
{
	printf(USAGE_STRING, program_name, VERSION, program_name, program_name);
	exit (0);
}

int main(int argc, char *argv[])
{
	char* program_name = argv[0];
	char* file_input_name = {0};
	char* file_output_name = {0};

	Program_options options = {0};

	if (argc < 2)
	{
		printf("%s: Input file not specified. Use -h or --help for more details.\n", program_name);
		return -1;
	}

	for (i32 arg = 1; arg < argc; arg++)
	{
		char c1 = argv[arg][0];
		char c2 = argv[arg][1];
		if (c1 == '-')
		{
			if (c2 == '-')
			{
				char* option = &argv[arg][2];
				if (strcmp(option, "help") == 0)
				{
					PrintUsageAndExit(program_name);
				}
				else if (strcmp(option, "print") == 0)
				{
					options.print_to_screen = true;
				}
				else if (strcmp(option, "debug") == 0)
				{
					options.debug_output = true;
				}
				else
				{
					printf("%s: Invalid argument '%s'. Use -h or --help for more details.\n", program_name, option);
					return -1;
				}
				continue;
			}
			else
			{
				if (strlen(argv[arg]) > 2) // Multiple arguments after a single '-' not currently supported.
				{
					printf("%s: Invalid argument '%s'. Use -h or --help for more details.\n", program_name, &argv[arg][1]);
					return -1;
				}

				switch (c2)
				{
				case 'h':
					PrintUsageAndExit(program_name);
				case 'p':
					options.print_to_screen = true;
					break;
				case 'd':
					options.debug_output = true;
					break;
				default:
					printf("%s: Invalid argument '%c'. Use -h or --help for more details.\n", program_name, c2);
					return -1;
				}
				continue;
			}
		}
		if (!file_input_name) // The first argument after the last option switch must be the input file.
		{
			file_input_name = argv[arg];
			continue;
		}
		// The last argument must be the output file.
		file_output_name = argv[arg];
		break; // No point in processing any additional arguments.
	}

	if (!file_input_name)
	{
		printf("No input file specified.\n");
		return -1;
	}

	if (!file_output_name && !options.print_to_screen)
	{
		printf("No output file specified.\n");
		return -1;
	}

	char* data = ReadEntireFile(file_input_name);
	if (!data)
	{
		printf("Could not open input file: %s\n", file_input_name);
		return -1;
	}

	Report_summary summary = {0};

	if (file_output_name)
	{
		FILE* stream_output;
		errno_t error;

		error = fopen_s(&stream_output, file_output_name, "w");
		if (error)
		{
			printf("Could not create output file: %s\n", file_output_name);
			return -1;
		}

		ParseClasses(data, stream_output, options, &summary);
		fclose(stream_output);
	}
	else
	{
		ParseClasses(data, NULL, options, &summary);
	}

	printf("Processed a total of %d classes (%d pages).\n", summary.num_classes, summary.num_pages);
	if (file_output_name)
	{
		printf("Output dumped to %s.\n", file_output_name);
	}

	return 0;
}
