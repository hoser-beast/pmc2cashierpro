#include <windows.h>
#include <stdio.h>

#include "utils.h"

#define VERSION "2024-11-25"

typedef struct
{
	bool print_to_screen;
	bool debug_output;
} Program_options;

typedef struct
{
	u32 num_accounts;
	u32 num_invoices;
	u32 num_pages;
	u32 total_owed;
} Report_summary;

typedef struct
{
	// char account_id[10]; // 9 + \0 // Not needed
	char credit_memo[4]; // 3 + \0 (what is this for?)
	char invoice_location[4]; // 2 + \0
	char payment_code[4]; // 2 + \0
	char invoice[8]; // 6 + \0
	char transaction_type[4]; // 3 + \0
	char reference[8]; // 6 + \0
	char date[10]; // 9 + \0
	char transaction_amount[12]; // 11 + \0
	char amount[12]; // 11 + \0
} Invoice;

void ParseCrossReferences(char* data, FILE* output_file, Program_options options, Report_summary* summary)
{
	size_t index			= 0;
	size_t line_start_index = 0;
	size_t line_position	= 0;
	i32 page_header_line	= 6; // Reports start with a header.
								 // After the first header is a customer location line that we should skip befor regular processing.

	char current_account[10] = {0};
	char* current_line = malloc(sizeof(char) * 60);

	Invoice invoice = {0};
	Invoice invoice_reset = {0};


	if (output_file && !options.debug_output)
	{
		fprintf(output_file, "Cust ID|Invoice|Date|Amount\n");
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
				page_header_line = 5;
				summary->num_pages++;
				continue;
			}

			line_start_index = index - line_position;
			invoice = invoice_reset;

			if (line_position < 60)
			{
				// This line is either the start of an account or the start of the summary.
				memcpy(current_line, &data[line_start_index], line_position + 1);
				current_line[line_position] = '\0';
				if (strstr(current_line, "Cust Loc:") != NULL)
				{
					break;
				}

				// This must be the start of an account.
				char* first_space = strchr(&data[line_start_index], ' ');
				if (first_space != NULL)
				{
					memcpy(current_account, &data[line_start_index], first_space - &data[line_start_index]);
				}
				line_position = 0;
				index++;
				summary->num_accounts++;
				continue;
			}
			if (data[line_start_index + 51] == '.')
			{
				line_position = 0;
				index++;
				continue;
			}
			FillTextFieldAndTrim(invoice.credit_memo, &data[line_start_index + 41], 3);
			FillTextFieldAndTrim(invoice.invoice_location, &data[line_start_index + 45], 2);
			FillTextFieldAndTrim(invoice.payment_code, &data[line_start_index + 48], 2);
			FillTextFieldAndTrim(invoice.invoice, &data[line_start_index + 51], 6);
			FillTextFieldAndTrim(invoice.transaction_type, &data[line_start_index + 59], 3);
			FillTextFieldAndTrim(invoice.reference, &data[line_start_index + 63], 6);
			FillTextFieldAndTrim(invoice.date, &data[line_start_index + 70], 8);
			FillTextFieldAndTrim(invoice.transaction_amount, &data[line_start_index + 108], 10);
			FillTextFieldAndTrim(invoice.amount, &data[line_start_index + 120], 10);


			bool balance_minus = false;
			if ((line_position > 130) && (data[index - 1] == '-'))
			{
				balance_minus = true;
			}

			char buffer[256] = {0};
			if (options.debug_output)
			{
				sprintf_s(buffer, sizeof(buffer),
						"%9s-00%32s%3s%3s%7s%5s %-6s%9s %39s%s %11s%s\n",
						current_account,
						invoice.credit_memo,
						invoice.invoice_location,
						invoice.payment_code,
						invoice.invoice,
						invoice.transaction_type,
						invoice.reference,
						invoice.date,

						invoice.transaction_amount,
						balance_minus ? "-" : "",
						invoice.amount,
						balance_minus ? "-" : ""
						);
			}
			else
			{
				sprintf_s(buffer, sizeof(buffer),
						"%s|%s|%s|%s%s\n",
						current_account,
						invoice.invoice,
						invoice.date,
						balance_minus ? "-" : "",
						invoice.amount
						);
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
			summary->num_invoices++;
			index++;
		}
	}
}

#define USAGE_STRING "\
%s %s\n\
John Hosick <john@atikokancastle.com>\n\n\
%s is used to process a ProfitMaster subsidiary report (RRT) report and output a\n\
pipe-delimited file of open invoices by customer for use in the conversion to CashierPRO.\n\n\
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

	printf("Processed a total of %d invoices ($%d) in %d customers (%d pages).\n",
			summary.num_invoices, summary.total_owed, summary.num_accounts, summary.num_pages);
	if (file_output_name)
	{
		printf("Output dumped to %s.\n", file_output_name);
	}
	return 0;
}
