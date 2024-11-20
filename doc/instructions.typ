= Overview

The purpose of this guide is to document the steps required to convert data from a ProfitMaster system to CashierPRO.
The data we require falls generally into two categories: inventory data and customer data.

CashierPRO has provided us with Microsoft Excel templates for inventory and customer information.
Present in these spreadsheets are the column headings (i.e., 'fields') that outline the data types as well as some detail regarding the expected format.
While the experts at CashierPRO can accommodate some variation in the format of the data,
we must still pay careful attention in ensuring that the information is correct and meets the requirements of CashierPRO for a functional POS system.

= Customers

According to the ProfitMaster documentation, an Excel file can be generated using RRL that contains all of the required fields.
Unfortunately, our ProfitMaster system is out-of-date (v4.7.0) thus we do not have the builtin option to export the customer data to an Excel document.
// @TODO: verify version number.
Instead, we must parse the text of three customer reports separately then combine specific data from each file to create a customer roster that meets the specifications outlined in the provided customer template.

The three customer reports we must generate are:

  - customer listing (provides us with the credit limit and account balance)
  - address list (provides the addresses, city, postal code, phone, account type, tax exemption, etc.)
  - memo list (contains notes, email addresses, First Nation status identity card details, etc.)

== Generating the Customer Reports

=== Customer Listing

+ At the main menu in ProfitMaster, type |R| |R| |L| (for Accounts Receivable → Report → Customer Listing).
+ Accept the default `**` in the *LOCATION* field.
+ In the *LIST OPTION ?* field, type `1` for "ACCOUNT LIST".
+ Press |END| or (|ENTER| through the rest of the options) to accept the remaining default values.
+ On the next screen, press `Y` to process all accounts.
+ Press |END| to accept the remaining default values.
+ In the Printer Selection Screen, press `F` for "file".
+ |ENTER| down to the *FILE NAME* field and type in a relevant filename like "ACCTMMDDYY.TXT" where "MMDDYY" is the month, day, and year, respectively.

=== Customer Addresses

+ At the main menu in ProfitMaster, type R R L.
+ Accept the default `**` in the *LOCATION* field.
+ In the *LIST OPTION ?* field, type `2` for "ADDRESS LIST".
+ Press |END| or (|ENTER| through the rest of the options) to accept the remaining default values.
+ On the next screen, press `Y` to process all accounts.
+ Press |END| to accept the remaining default values.
+ In the Printer Selection Screen, press `F` for "file".
+ |ENTER| down to the *FILE NAME* field and type in a relevant filename like "ADDRMMDDYY.TXT" where "MMDDYY" is the month, day, and year, respectively.

=== Customer Memos

+ At the main menu in ProfitMaster, type R R L.
+ Accept the default `**` in the *LOCATION* field.
+ In the *LIST OPTION ?* field, type `5` for "RUM/SUM MEMO LIST".
+ Press |END| or (|ENTER| through the rest of the options) to accept the remaining default values.
+ On the next screen, press `Y` to process all accounts.
+ Press |END| to accept the remaining default values.
+ In the Printer Selection Screen, press `F` for "file".
+ |ENTER| down to the *FILE NAME* field and type in a relevant filename like "MEMOMMDDYY.TXT" where "MMDDYY" is the month, day, and year, respectively.

== Downloading the Generated Reports

+ In Windows Explorer, create a folder in the "Documents" inside your home directory (i.e., "PMC-DATA-MMDDYY" where "MMDDYY" is the month, day, and year, respectively).
  For example, if the date was November 20, 2024 the absolute path would be `C:\Users\<USER>\Documents\PMC-DATA-112024`.
+ Launch your FTP client (WinSCP).
+ Connect to the ProfitMaster FTP server. (In our case the IP address is 10.25.25.2.)
+ Specify the provided FTP user name and password (same as your server's ZUU/root password).
+ Navigate to the `/pmc/db2/sp01/hist/ascii` directory.
// @TODO: verify the path.
+ Transfer the three files containing the reports generated in previously into the directory created above.

== Parsing the Reports

+ In a Windows Command Prompt (press |WIN|, type '`cmd`', then press |ENTER|).
+ Navigate to the directory containing the file conversion utilities.
// @TODO: document the building of these tools, etc.
+ Run the `customers` program for each type of customer report to generate a pipe-delimited file suitable for import into Excel.

  The customer program has the following general syntax:
  ```
  customers <report-type> <report-file> <output-file>
  ```

  In our example, the following commands would be used to parse the three reports generated on November 20, 2024:

  ```
  customers account %USERPROFILE%\Documents\ACCT112024.TXT accounts.txt
  customers address %USERPROFILE%\Documents\ADDR112024.TXT addresses.txt
  customers memo %USERPROFILE%\Documents\MEMO112024.TXT memos.txt
  ```

  Ensure that the summary printed upon completion of each parse matches the totals on bottom of the original reports.

== Creating the Customer Spreadsheet

Now that the necessary customer information is parsed, follow these steps to import the data into a spreadsheet conforming to the template provided by CashierPRO.

=== Importing Customer Addresses

+ Open Microsoft Excel.
+ Create a new spreadsheet and choose a blank workbook.
+ Go to the *Data* ribbon at the top of the window and click *From Text/CSV*.
+ Browse to the location of the parsed files.
+ Select the address file (`address.txt`) file then click *Open*.
+ You should see a dialog showing a sample of the data in tabular form.
  The '|' delimiter should automatically be detected. Click *Load*.
+ If the headers shown in the sample read "Column1", "Column2", etc. click *Transform Data*.
  Otherwise, if the column headers are the field name (i.e., "Cust ID", "First Name") click *Load*.
+ In the Power Query Editor window (if you chose to transform the data), click *Use First Row as Headers* then close the window.
  Click *Keep* to save your changes.
+ A worksheet with a table will be automatically created with data.

=== Importing Customer Account Balances

+ Go back to the *Data* ribbon at the top of the window and click *From Text/CSV*.
+ Select the address file (`accounts.txt`) file then click *Open*.
  The '|' delimiter should automatically be detected. Click *Load*.
+ You should see a dialog showing a sample of the data in tabular form.
  The '|' delimiter should automatically be detected. Click *Load*.
+ If the headers shown in the sample read "Column1", "Column2", etc. click *Transform Data*.
  Otherwise, if the column headers are the field name (i.e., "Cust ID", "First Name") click *Load*.
+ In the Power Query Editor window (if you chose to transform the data), click *Use First Row as Headers* then close the window.
  Click *Keep* to save your changes.

=== Importing Customer Memos

+ Go to the *Data* ribbon at the top of the window and click *From Text/CSV*.
+ Browse to the location of the parsed files.
+ Select the address file (`memos.txt`) file then click *Open*.
+ You should see a dialog showing a sample of the data in tabular form.
  The '|' delimiter should automatically be detected. Click *Load*.
+ If the headers shown in the sample read "Column1", "Column2", etc. click *Transform Data*.
  Otherwise, if the column headers are the field name (i.e., "Cust ID", "First Name") click *Load*.
+ In the Power Query Editor window (if you chose to transform the data), click *Use First Row as Headers* then close the window.
  Click *Keep* to save your changes.

=== Merging the Worksheets

+ We should have three worksheets in the Excel workbook.
+ Save the file now (*File → Save As*) and save it in the same directory as the reports.
+ Go to the *Data* ribbon at the top of the window and click *From Text/CSV*.
+ Click *Get Data → Combine Queries → Merge*.
+ In the Merge window, for the first table, select the addresses worksheet.
+ Click the _Cust ID_ column in the first table to select it.
+ In the second table, select the accounts worksheet.
+ Click the _Cust ID_ column in the second table to select it.
+ Accept the Join kind as "Left Outer" and click *OK*.
+ In the Power Query Editor window, click small box on the right of the highlighted column header. (It should be a column called 'accounts'.)
  Uncheck _Use original column name as prefix_, uncheck the _(Select All Columns)_ and choose only the _Credit Limit_ and _Current Balance_ columns.
  Click *OK*
+ In the top of the Power Query Editor, click *Merge Queries*.
+ In the first table, select the _Cust ID_ column.
+ In the Merge window, select in the second table the memos worksheet.
+ In the second table, select the _Cust ID_ column.
+ Accept the Join kind as "Left Outer" and click *OK*.
+ In the Power Query Editor window, click small box on the right of the highlighted column header. (It should be a column called 'memos'.)
  Ensure that _Use original column name as prefix_ is unchecked, uncheck the _(Select All Columns)_ and choose only the _Memo_ column.
  Click *OK*
+ Click the close button and choose *Keep* to save the changes.
+ Save the file (*File → Save*).

You should now have a worksheet that combines the three worksheets together.

=== Final Touches

+ Create a new worksheet and rename it to "Export".
+ Copy the merged sheet. Paste it as text values into the newly-created Export worksheet.
+ With the data still selected, *Format as Table*.
+ Create a column to the right of _Tax Exemption_ and type "Exemption Info" in the column heading.
+ Move any tax exemption info from the memo field to the _Tax Exemption_ field.
+ Create a column to the right of the newly-created _Exemption Info_ and type "Customer Type" in the column heading.










= Inventory
