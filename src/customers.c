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
	u32 num_accounts;
	u32 num_pages;
} Report_summary;

typedef struct
{
	char line_1[30]; // actually 27
	char line_2[30]; // actually 27
	char city[20]; // 17 + \0
	char province[3]; // 2 + \0
	char postal_code[12]; // 10 + \0
} Customer_address;

typedef struct Customer_memo
{
	char account_id[10]; // 9 + \0
	char rum_line_1[26]; // 24 + \0
	char rum_line_2[26];
	char rum_line_3[26];
	char rum_line_4[26];

	char sum_line_1[41]; // 40 + \0
	char sum_line_2[41];
	char sum_line_3[41];
} Customer_memo;

typedef struct Customer_account
{
	char location[4];
	char id[10];
	char type;
	char tax_authority[5];
	char price_level;
	char payment_code[4];
	char original_name[30]; // needed to for parse output to equal original.
	char first_name[30];
	char last_name_or_company_name[30];
	char phone_number[16];
	char fax_number[16];
	char credit_limit[8];
	char balance[12];
	char balance_credit[4];
	char ytd_sales[12];
	char ytd_sales_credit[4];
	char ytd_fin_charges[16];
	char date_account_setup[9];
	char date_last_payment[9];
	char date_last_purchase[9];
	Customer_address address;
	Customer_memo memo;
} Customer_account;

typedef enum
{
	account,
	address,
	memo
} Report_type;

void ParseAccountBalances(char* data, FILE* output_file, Program_options options, Report_summary* summary)
{
	size_t index			= 0;
	size_t line_start_index = 0;
	size_t line_position	= 0;
	i32 page_header_line	= 8; // Reports start with a header.

	Customer_account account = {0};
	Customer_account account_reset = {0};

	if (output_file && !options.debug_output)
	{
		fprintf(output_file, "Cust ID|Credit Limit|Current Balance\n");
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

			FillTextFieldAndTrim(account.location, &data[line_start_index], 2);
			FillTextFieldAndTrim(account.id, &data[line_start_index + 3], 9);
			account.type = data[line_start_index + 13];
			FillTextFieldAndTrim(account.tax_authority, &data[line_start_index + 16], 4);
			account.price_level = data[line_start_index + 21];
			FillTextFieldAndTrim(account.payment_code, &data[line_start_index + 23], 2);
			FillTextFieldAndTrim(account.last_name_or_company_name, &data[line_start_index + 26], 26);
			FillTextFieldAndTrim(account.phone_number, &data[line_start_index + 53], 8);
			FillTextFieldAndTrim(account.credit_limit, &data[line_start_index + 62], 7);
			FillTextFieldAndTrim(account.balance, &data[line_start_index + 70], 10);
			FillTextFieldAndTrim(account.balance_credit, &data[line_start_index + 80], 2);
			FillTextFieldAndTrim(account.ytd_sales, &data[line_start_index + 82], 8);
			FillTextFieldAndTrim(account.ytd_sales_credit, &data[line_start_index + 90], 2);
			FillTextFieldAndTrim(account.ytd_fin_charges, &data[line_start_index + 92], 12);
			FillTextFieldAndTrim(account.date_account_setup, &data[line_start_index + 104], 8);
			if (line_position > 113)
			{
				FillTextFieldAndTrim(account.date_last_payment, &data[line_start_index + 114], MIN(line_position - 114, 8));
			}
			if (line_position > 123)
			{
				FillTextFieldAndTrim(account.date_last_purchase, &data[line_start_index + 124], MIN(line_position - 124, 8));
			}

			if (line_position == 69) // @HACK: ensure that we don't include summary lines in our account total.
			{
				break;
			}

			char buffer[256] = {0};
			if (options.debug_output)
			{
				sprintf_s(buffer, sizeof(buffer),
			        "%2s %9s-%c  %-4s %c %s %-26s %8s %7s %10s%s %10s%s %11s %-9s %-9s %8s\n",
			        account.location,
			        account.id,
			        account.type,
			        account.tax_authority,
			        account.price_level,
			        account.payment_code,
			        account.last_name_or_company_name,
			        account.phone_number,
			        account.credit_limit,
			        account.balance,
			        account.balance_credit,
			        account.ytd_sales,
			        account.ytd_sales_credit,
			        account.ytd_fin_charges,
			        account.date_account_setup,
			        account.date_last_payment[0] != '\0' ? account.date_last_payment : "",
			        account.date_last_purchase[0] != '\0' ? account.date_last_purchase : ""
				   	);
			}
			else
			{
				bool limit_zero = false;
				bool balance_minus = false;
				bool balance_zero = false;
				if (account.credit_limit[0] == '\0')
				{
					limit_zero = true;
				}
				if (account.balance_credit[0] == 'C' && account.balance_credit[1] == 'R')
				{
					balance_minus = true;
				}
				if (account.balance[0] == '.' && account.balance[1] == '0' && account.balance[2] == '0')
				{
					balance_zero = true;
				}
				sprintf_s(buffer, sizeof(buffer),
					"%s|%s|%s%s\n",
					account.id,
					limit_zero ? "0" : account.credit_limit,
					balance_minus ? "-" : "",
					balance_zero ? "0" : account.balance);
			}

			if (output_file)
			{
				fprintf(output_file, "%s", buffer);
			}
			if (options.print_to_screen)
			{
				printf("%s", buffer);
			}

			account = account_reset;
			line_position = 0;

			summary->num_accounts++;
			index++;
		}
	}
}

void ParseAccountAddresses(char* data, FILE* output_file, Program_options options, Report_summary* summary)
{
	size_t index			= 0;
	size_t line_start_index = 0;
	size_t line_position	= 0;
	i32 semicolon_position  = 0;
	u32 page_header_line	= 7;
	u32 account_line		= 0;

	Customer_account account = {0};
	Customer_account empty_account = {0};

	// Output table headers
	if (output_file && !options.debug_output)
	{
		fprintf(output_file, "Cust ID|First Name|Last Name or Company Name|Address1|Address2|City|Prov|Postal Cd|PhoneNo|FaxNo|Tax Exemption|House Acct\n");
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
			if (page_header_line > 0)
			{
				while (data[index] && (page_header_line > 0)) // skip header lines
				{
					if (data[index] == '\n')
					{
						page_header_line--;
					}
					index++;
				}
				account_line = 1;
				summary->num_pages++;
				/*index++;*/
				continue;
			}

			line_start_index = index - line_position;

			if (account_line == 1)
			{
				FillTextFieldAndTrim(account.id, &data[line_start_index], 9);
				account.type = data[line_start_index + 10];
				FillTextFieldAndTrim(account.tax_authority, &data[line_start_index + 12], 4);
				account.price_level = data[line_start_index + 17];
				FillTextFieldAndTrim(account.payment_code, &data[line_start_index + 19], 2);
				FillTextFieldAndTrim(account.last_name_or_company_name, &data[line_start_index + 22], 27);
				FillTextFieldAndTrim(account.original_name, &data[line_start_index + 22], 27);

				semicolon_position = FindCharInString(account.last_name_or_company_name, ';');
				if (semicolon_position >= 0)
				{
					memcpy(account.first_name, account.last_name_or_company_name, sizeof(char) * semicolon_position);
					size_t last_name_length = strlen(account.last_name_or_company_name);
					memmove(account.last_name_or_company_name, account.last_name_or_company_name + semicolon_position + 1, last_name_length - semicolon_position);
					account.last_name_or_company_name[last_name_length - semicolon_position] = '\0';
				}

				if (line_position > 128)
					FillTextFieldAndTrim(account.phone_number, &data[line_start_index + 118], MIN(line_position - 17, 17));

				account_line++;
			}
			else if (account_line == 2)
			{
				if (line_position > 23)
					FillTextFieldAndTrim(account.address.line_1, &data[line_start_index + 23], MIN(line_position - 23, 27));

				if (line_position > 51)
					FillTextFieldAndTrim(account.address.line_2, &data[line_start_index + 51], MIN(line_position - 51, 27));

				if (line_position > 79)
					FillTextFieldAndTrim(account.address.city, &data[line_start_index + 79], MIN(line_position - 79, 17));

				if (line_position > 100)
					FillTextFieldAndTrim(account.address.province, &data[line_start_index + 100], MIN(line_position - 100, 2));

				if (line_position > 103)
					FillTextFieldAndTrim(account.address.postal_code, &data[line_start_index + 103], MIN(line_position - 103, 10));

				if (line_position > 128)
					FillTextFieldAndTrim(account.fax_number, &data[line_start_index + 118], MIN(line_position - 17, 14));

				if (account.id[0] == '\0') // break loop when out of records.
				{
					break;
				}

				char buffer[512] = {0};
				if (options.debug_output)
				{
					sprintf_s(buffer, sizeof(buffer),
				        "%9s %c %-4s %c %s %-95s %s\n                       %-27s %-27s %-20s %-2s %-10s FAX %s\n",
				        account.id,
				        account.type,
				        account.tax_authority,
				        account.price_level,
				        account.payment_code,
				        account.original_name,
				        account.phone_number[0] == '\0' ? "(807) 597-" : account.phone_number,
				        account.address.line_1,
				        account.address.line_2,
				        account.address.city,
				        account.address.province,
				        account.address.postal_code,
				        account.fax_number[0] == '\0' ? "(807) 597-" : account.fax_number
					   	);
				}
				else
				{
					char* tax_exemptions = {0};
					if (strcmp(account.tax_authority, "EXEM") == 0)
					{
						tax_exemptions = "Exempt";
					}
					else if (strcmp(account.tax_authority, "ONFN") == 0)
					{
						tax_exemptions = "GST";
					}
					else // else if (strcmp(account.tax_authority, "ON") == 0)
					{
						tax_exemptions = "Tax";
					}

					sprintf_s(buffer, sizeof(buffer),
				        "%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
				        account.id,
				        account.first_name,
				        account.last_name_or_company_name,
				        account.address.line_1,
				        account.address.line_2,
				        account.address.city,
				        account.address.province,
				        account.address.postal_code,
				        account.phone_number,
				        account.fax_number,
				        tax_exemptions,
				        account.type == 'O' ? "Yes" : "No"
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

				account = empty_account; // reset struct.
				account_line = 1;

				summary->num_accounts++;
				if ((summary->num_accounts % 25) == 0) // each page contains exactly 25 accounts.
				{
					page_header_line = 7;
				}
			}

			line_position = 0;
			index++;
		}
	}
}

void ParseAccountMemos(char* data, FILE* output_file, Program_options options, Report_summary* summary)
{
	size_t index			= 0;
	size_t line_start_index = 0;
	size_t line_position	= 0;
	u32 page_header_line	= 6; // Reports start with a header.
	u32 account_line		= 0;

	Customer_account account = {0};
	Customer_account empty_account = {0};

	if (output_file && !options.debug_output)
	{
		fprintf(output_file, "Cust ID|Memo\n");
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
			if (page_header_line > 0)
			{
				while (data[index] && (page_header_line > 0)) // Skip header lines.
				{
					if (data[index] == '\n')
					{
						page_header_line--;
					}
					index++;
				}
				account_line = 1;
				summary->num_pages++;
				continue;
			}

			line_start_index = index - line_position;

			switch (account_line)
			{
				case 1:
				{
					FillTextFieldAndTrim(account.id, &data[line_start_index + 1], 9);

					if (line_position > 23) // Must have at least one note on the first line.
						FillTextFieldAndTrim(account.memo.rum_line_1, &data[line_start_index + 23], MIN(line_position - 23, 25));

					if (line_position > 49) // Must have a SUM memo.
						FillTextFieldAndTrim(account.memo.sum_line_1, &data[line_start_index + 49], line_position - 49);

					break;
				}
				case 2:
				{
					if (line_position > 23)
						FillTextFieldAndTrim(account.memo.rum_line_2, &data[line_start_index + 23], MIN(line_position - 23, 25));

					if (line_position > 49) // Must have a SUM memo.
						FillTextFieldAndTrim(account.memo.sum_line_2, &data[line_start_index + 49], line_position - 49);

					break;
				}
				case 3:
				{
					if (line_position > 23)
						FillTextFieldAndTrim(account.memo.rum_line_3, &data[line_start_index + 23], MIN(line_position - 23, 25));

					if (line_position > 49) // Must have a SUM memo.
						FillTextFieldAndTrim(account.memo.sum_line_3, &data[line_start_index + 49], line_position - 49);

					break;
				}
				case 4:
				{
					if (line_position > 23)
						FillTextFieldAndTrim(account.memo.rum_line_4, &data[line_start_index + 23], MIN(line_position - 23, 25));
				}
			}

			account_line++;
			if (account_line > 4)
			{
				char buffer[512] = {0}; // This should be big enough for even the longest memo.
				if (options.debug_output)
				{
					sprintf_s(buffer, sizeof(buffer),
							"%10s-00          %-25s %s\n                       %-25s %s\n                       %-25s %s\n                       %s\n",
							account.id,
							account.memo.rum_line_1,
							account.memo.sum_line_1,
							account.memo.rum_line_2,
							account.memo.sum_line_2,
							account.memo.rum_line_3,
							account.memo.sum_line_3,
							account.memo.rum_line_4
							);
				}
				else
				{
					sprintf_s(buffer, sizeof(buffer),
							"%s|%s %s %s %s %s %s %s\n",
							account.id,
							account.memo.rum_line_1,
							account.memo.rum_line_2,
							account.memo.rum_line_3,
							account.memo.rum_line_4,
							account.memo.sum_line_1,
							account.memo.sum_line_2,
							account.memo.sum_line_3
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

				account = empty_account; // Reset struct.
				account_line = 1;

				summary->num_accounts++;
				if ((summary->num_accounts % 13) == 0) // Each page contains exactly 13 accounts.
				{   // Next line will be the start of a header.
					page_header_line = 6;
				}
			}

			line_position = 0;
			index++;
		}
	}
}

#define USAGE_STRING "\
%s %s\n\
John Hosick <john@atikokancastle.com>\n\n\
%s is used to process a ProfitMaster IRL customer report and output a\n\
pipe-delimited file for use in the conversion to CashierPRO.\n\n\
USAGE: %s [OPTIONS] <reporttype> <inputfile> [outputfile]\n\
  REPORT TYPES:\n\
    account         Process a customer account listing report.\n\
    address         Process a customer address report.\n\
    memo            Process a customer memo report.\n\
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
	
	Report_type report_type = -1;

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
		if (report_type < 0)
		{
			char* type = argv[arg];
			if (strcmp(type, "account") == 0)
			{
				report_type = account;
			}
			else if (strcmp(type, "address") == 0)
			{
				report_type = address;
			}
			else if (strcmp(type, "memo") == 0)
			{
				report_type = memo;
			}
			else
			{
				printf("%s: Invalid report type '%s'. Use -h or --help for more details.\n", program_name, type);
				return -1;
			}
			continue;
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

	switch (report_type)
	{
		case account:
			ParseAccountBalances(data, stream_output, options, &summary);
			printf("Processed a total of %d accounts (%d pages).\n", summary.num_accounts, summary.num_pages);
			break;
		case address:
			ParseAccountAddresses(data, stream_output, options, &summary);
			printf("Processed a total of %d addresses (%d pages).\n", summary.num_accounts, summary.num_pages);
			break;
		case memo:
			ParseAccountMemos(data, stream_output, options, &summary);
			printf("Processed a total of %d memos (%d pages).\n", summary.num_accounts, summary.num_pages);
			break;
	}

	if (file_output_name)
	{
		fclose(stream_output);
		printf("Output dumped to %s.\n", file_output_name);
	}

	return 0;
}
