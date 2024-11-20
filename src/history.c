#include <windows.h>
#include <stdio.h>

#include "utils.h"

#define VERSION "2024-11-19"

typedef struct
{
	bool print_to_screen;
	bool debug_output;
} Program_options;

typedef struct
{
	u32 num_products;
	u32 num_pages;
} Report_summary;

typedef struct
{
	// char class[8]; // 4 + \0 (not in this report)
	char sku[12]; // 11 + \0
	char description_1[26]; // 25 + \0
	char description_2[26]; // 25 + \0
	char location[4]; // 2 + \0
	char avg_cost[12]; // 10 + \0
	char last_cost[12]; // 10 + \0
	char last_received[10]; // 8 + \0
	char retail_price[12]; // 10 + \0
	char available[8]; // 7 + \0
	char reserved[8]; // 7 + \0
	char on_order[8]; // 7 + \0
	char order_point[8]; // 7 + \0
	char order_quantity[8]; // 7 + \0
	char current_period[10]; // 8 + \0
	char vendor[8]; // 6 + \0
	i32  history_periods[24];
	i32  year_1_sales;
	i32  year_2_sales;
} Product;

/*	NOTES
	=====

	- An IRH report starts with a blank line followed by a history calendar for the business.
	- The next blank line separates the calendar from the first page header.
	- It seems that the page headers are not consistent: The first eleven page headers begin with an empty
	  line followed by six lines of header text; the rest have an empty line in the middle of the header.
	- Each product requires three lines unless there is no history, in which case two are required.
*/

void ParseProductHistory(char* data, FILE* output_file, Program_options options, Report_summary* summary)
{
	size_t index			= 0;
	size_t line_start_index = 0;
	size_t line_position	= 0;
	i32 page_header_line	= 7;
	i32 empty_lines_to_skip	= 2; // Used to skip history calendar at start of report.
	i32 product_line		= 0;

	Product product = {0};
	Product product_reset = {0};

	// Skip over history calendar at the start of the report.
	while (data[index])
	{
		if ((data[index] == '\n'))// && (line_position == 0))
		{
			if (line_position == 0)
			{
				empty_lines_to_skip--;
			}
			if (!empty_lines_to_skip)
			{
				break;
			}
			line_position = 0;
			index++;
			continue;
		}
		index++;
		line_position++;
	}

	// @TODO: add option to specify what period is current or period 1 and subtract back in time.
	// Put in headers the month names instead of 'P1', 'P2', etc.?
	if (output_file && !options.debug_output)
	{
		fprintf(output_file, "SKU|CURRENT|P1|P2|P3|P4|P5|P6|P7|P8|P9|P10|P11|P12|P13|P14|P15|P16|P17|P18|P19|P20|P21|P22|P23|P24\n");
	}

	// Now start parsing report.
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
				page_header_line = 7;
				product_line = 1;
				summary->num_pages++;
				continue;
			}

			line_start_index = index - line_position;
			char buffer[512] = {0};

			if (data[line_start_index] != ' ')
			{
				if (data[line_start_index] == '=') // Reached the report footer/summary.
				{
					break;
				}
				FillTextFieldAndTrim(product.sku, &data[line_start_index], 11);
				FillTextFieldAndTrim(product.description_1, &data[line_start_index + 12], 25);
				FillTextFieldAndTrim(product.location, &data[line_start_index + 38], 2);
				FillTextFieldAndTrim(product.avg_cost, &data[line_start_index + 40], 10);
				FillTextFieldAndTrim(product.last_cost, &data[line_start_index + 50], 10);
				FillTextFieldAndTrim(product.last_received, &data[line_start_index + 61], 8);
				FillTextFieldAndTrim(product.retail_price, &data[line_start_index + 70], 10);
				FillTextFieldAndTrim(product.available, &data[line_start_index + 80], 7);
				FillTextFieldAndTrim(product.reserved, &data[line_start_index + 87], 7);
				FillTextFieldAndTrim(product.on_order, &data[line_start_index + 94], 7);
				FillTextFieldAndTrim(product.order_point, &data[line_start_index + 101], 7);
				FillTextFieldAndTrim(product.order_quantity, &data[line_start_index + 108], 7);
				FillTextFieldAndTrim(product.current_period, &data[line_start_index + 115], 8);
				if (line_position > 123)
				{
					FillTextFieldAndTrim(product.vendor, &data[line_start_index + 124], 6);
				}

				product_line++;
			}
			else if (product_line == 2)
			{
				FillTextFieldAndTrim(product.description_2, &data[line_start_index + 2], 25);

				product_line++;

				if ((line_position > 61) && (data[index - 1] != '*')) // @TODO: is the check for '*' even necessary?
				{
					char sales_for_current_period[9] = {0};
					for (i32 current_period = 0; current_period < 12; current_period++)
					{

						FillTextFieldAndTrim(sales_for_current_period, &data[line_start_index + 27 + current_period * 8], 8);
						product.history_periods[current_period] = atoi(sales_for_current_period);
						product.year_1_sales += product.history_periods[current_period];
					}
				}
				else
				{
					// Must be no history records found.

					// Print just the first line and description from second line.
					if (options.debug_output)
					{
						sprintf_s(buffer, sizeof(buffer),
							"%-11s %-25s %2s%10s%10s %8s %10s%7s%7s%7s%7s%7s%8s %6s\n  %-25s  *** NO HISTORY RECORDS FOUND ***\n",
					        product.sku,
					        product.description_1,
					        product.location,
					        product.avg_cost,
					        product.last_cost,
					        product.last_received,
					        product.retail_price,
					        product.available,
					        product.reserved,
					        product.on_order,
					        product.order_point,
					        product.order_quantity,
					        product.current_period,
					        product.vendor,
					        product.description_2
					        );
					}
					else
					{
						sprintf_s(buffer, sizeof(buffer),
							"%s|%s|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0\n",
					        product.sku,
					        product.current_period);
					}

					product_line = 1;
					product = product_reset;
					summary->num_products++;
				}
			}
			else if (product_line == 3)
			{
				char sales_for_current_period[9] = {0};
				for (i32 current_period = 12; current_period < 24; current_period++)
				{
					FillTextFieldAndTrim(sales_for_current_period, &data[line_start_index + 27 + (current_period - 12) * 8], 8);
					product.history_periods[current_period] = atoi(sales_for_current_period);
					product.year_2_sales += product.history_periods[current_period];
				}

				size_t offset = 0;
				size_t written = 0;
				if (options.debug_output)
				{
					written = sprintf_s(buffer, sizeof(buffer),
							"%-11s %-25s %2s%10s%10s %8s %10s%7s%7s%7s%7s%7s%8s %6s\n  %-25s",
							product.sku,
							product.description_1,
							product.location,
							product.avg_cost,
							product.last_cost,
							product.last_received,
							product.retail_price,
							product.available,
							product.reserved,
							product.on_order,
							product.order_point,
							product.order_quantity,
							product.current_period,
							product.vendor,
							product.description_2
							);
					offset = written;
					if ((written < 0) || (size_t)written >= sizeof(buffer) - offset)
					{
						printf("Error: Buffer size exceeded!\n");
						exit (-1);
					}
					for (i32 year = 0; year < 2; year++)
					{
						for (i32 period = 0; period < 12; period++)
						{
							written = sprintf_s(buffer + offset, sizeof(buffer) - offset, "%8d", product.history_periods[(year * 12) + period]);
							if (written < 0 || (size_t)written >= sizeof(buffer) - offset)
							{
								printf("Error: Buffer size exceeded!\n");
								exit (-1);
							}
							offset += written;
						}
						if (year == 0)
						{
							written = sprintf_s(buffer + offset, sizeof(buffer) - offset, "%8d\n                           ", product.year_1_sales);
							if (written < 0 || (size_t)written >= sizeof(buffer) - offset)
							{
								printf("Error: Buffer size exceeded!\n");
								exit (-1);
							}
							offset += written;
						}
						else
						{
							written = sprintf_s(buffer + offset, sizeof(buffer) - offset, "%8d\n", product.year_2_sales);
							if (written < 0 || (size_t)written >= sizeof(buffer) - offset)
							{
								printf("Error: Buffer size exceeded!\n");
								exit (-1);
							}
							offset += written;
						}
					}
				}
				else
				{
					written = sprintf_s(buffer, sizeof(buffer),
							"%s|%s", product.sku, product.current_period);
					offset = written;
					if (written < 0 || (size_t)written >= sizeof(buffer) - offset)
					{
						printf("Error: Buffer size excedded!\n");
						exit (-1);
					}
					for (i32 period = 0; period < 24; period++)
					{
						written = sprintf_s(buffer + offset, sizeof(buffer) - offset, "|%d", product.history_periods[period]);
						if (written < 0 || (size_t)written >= sizeof(buffer) - offset)
						{
							printf("Error: Buffer size exceeded!\n");
							exit (-1);
						}
						offset += written;
					}
					written = sprintf_s(buffer + offset, sizeof(buffer) - offset, "\n");
				}

				product = product_reset;
				product_line = 1;
				summary->num_products++;
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
			index++;
		}
	}
}

#define USAGE_STRING "\
%s %s\n\
John Hosick <john@atikokancastle.com>\n\n\
%s is used to process a ProfitMaster IRH product history report and output a\n\
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
	FILE* stream_output = NULL;

	if (file_output_name)
	{
		errno_t error;

		error = fopen_s(&stream_output, file_output_name, "w");
		if (error)
		{
			printf("Could not create output file: %s\n", file_output_name);
			return -1;
		}
	}

	ParseProductHistory(data, stream_output, options, &summary);
	printf("Processed a total of %d products (%d pages).\n", summary.num_products, summary.num_pages);

	if (file_output_name)
	{
		fclose(stream_output);
		printf("Output dumped to %s.\n", file_output_name);
	}

	return 0;
}
