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
	u32 num_products;
	u32 num_xrefs;
	u32 num_pages;
} Report_summary;

typedef struct Product_reference
{
	char class[8]; // 4 + \0
	char product_id[12]; // 11 + \0
	char description_1[26]; // 25 + \0
	char vendor[8]; // 6 + \0
	char reference[16]; // 15 + \0
} Product_reference;

void ParseCrossReferences(char* data, FILE* output_file, Program_options options, Report_summary* summary)
{
	size_t index			= 0;
	size_t line_start_index = 0;
	size_t line_position	= 0;
	i32 page_header_line	= 7; // Reports start with a header.

	Product_reference xref = {0};
	Product_reference xref_reset = {0};

	char current_class[8];
	char current_sku[12];
	char current_description[26];
	char current_vendor[8];
	size_t current_class_length = 0;
	size_t current_vendor_length = 0; // @TODO not really used yet
	size_t description_length = 0;
	size_t current_sku_length = 0;

	if (output_file && !options.debug_output)
	{
		fprintf(output_file, "SKU Number|UPC\n");
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
			if (line_position == 0) // @BUG: Found issue around line 57,002 where the IRX reports generated do not put space before the header!
			{						// will need another way of parsing these files if no manual fiddling is to be required.
				// loop over header lines
				while (data[index] && (page_header_line > 0)) // data[index] != '\0' is necessary in the case the report ends with a header.
				{
					if (data[index] == '\n')
					{
						page_header_line--;
					}
					index++;
				}
				page_header_line = 7;
				summary->num_pages++;
				continue;
			}

			line_start_index = index - line_position;
			xref = xref_reset;

			if ((line_position > 66) && (data[line_start_index + 66] != ' '))
			{	// @HACK: Don't count report footer as product.
				break;
				// printf("------------------------------------");
			}

			if (data[line_start_index + 2] != ' ')
			{
				current_class_length = FillTextFieldAndTrim(xref.class, &data[line_start_index + 2], 4);
				memcpy(current_class, xref.class, current_class_length);
			}
			if (data[line_start_index + 8] != ' ') // check if sku is present on line.
			{
				current_sku_length = FillTextFieldAndTrim(xref.product_id, &data[line_start_index + 8], 11);
				memcpy(current_sku, xref.product_id, current_sku_length);
				summary->num_products++;
			}

			if (data[line_start_index + 21] != ' ') // check if description is present on line.
			{
				// FillTextFieldAndTrim(product.description_1, &data[line_start_index + 21], 25);
				description_length = FillTextFieldAndTrim(xref.description_1, &data[line_start_index + 21], 25);
				memcpy(current_description, xref.description_1, description_length);
			}
			FillTextFieldAndTrim(xref.reference, &data[line_start_index + 48], MIN(line_position - 48, 15));

			if (line_position > 70)
			{
				current_vendor_length = FillTextFieldAndTrim(xref.vendor, &data[line_start_index + 70], 6);
				memcpy(current_vendor, xref.vendor, current_vendor_length);
			}

			char buffer[256] = {0};
			if (options.debug_output)
			{
				sprintf_s(buffer, sizeof(buffer),
					"  %-4s  %-11s  %-25s  %-21s %6s\n",
					xref.class,
					xref.product_id,
					xref.description_1,
					xref.reference,
					xref.vendor);
			}
			else
			{
				sprintf_s(buffer, sizeof(buffer),
					"%s|%s\n",
					// "%s|%s|%s|%s|%s\n",
					current_sku,
					// current_description,
					// current_class,
					// current_vendor,
					xref.reference);
			}

			if (output_file)
			{
				fprintf(output_file, "%s", buffer);
			}
			if (options.print_to_screen)
			{
				printf("%s", buffer);
			}

			line_position = 0;
			summary->num_xrefs++;
			index++;
		}
	}
}

#define USAGE_STRING "\
%s %s\n\
John Hosick <john@atikokancastle.com>\n\n\
%s is used to process a ProfitMaster IRX cross-reference report and output a\n\
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

		ParseCrossReferences(data, stream_output, options, &summary);
		fclose(stream_output);
	}
	else
	{
		ParseCrossReferences(data, NULL, options, &summary);
	}

	printf("Processed a total of %d cross-references in %d products (%d pages).\n", summary.num_xrefs, summary.num_products, summary.num_pages);
	if (file_output_name)
	{
		printf("Output dumped to %s.\n", file_output_name);
	}
	return 0;
}
