namespace BorealOS.Utilities
{
    public class StrUtils
    {
        /* VARIABLES */
        public static string SpecialCharacters = "`~!@#$%^&*()_+-=[]\\{}|;':\",../<>?";


        /* FUNCTIONS */
        // Return a new string with the first occurrence of a term replaced with another string
        public static string ReplaceFirstOccurrence(string Term, string Replacement, string OriginalStr)
        {
            int TermIndex = OriginalStr.IndexOf(Term);

            if (TermIndex < 0)
            {
                return OriginalStr;
            }

            return OriginalStr.Substring(0, TermIndex) + Replacement + OriginalStr.Substring(TermIndex + Term.Length);
        }

        public static int CharOccurrencesInStr(char Character, string Str)
        {
            int Count = 0;

            for (int I = 0; I < Str.Length; I++)
            {
                if (Str[I] == Character)
                {
                    Count++;
                }
            }

            return Count;
        }

        // Check if a character is alphanumeric by checking its ASCII value
        public static bool IsCharAlphaNumeric(char CharToCheck)
        {
            // Numeric characters are 48 -> 57
            if (CharToCheck < 48)
            {
                return false;
            }

            // Capital characters are 65 -> 90
            if (CharToCheck > 57 && CharToCheck < 65)
            {
                return false;
            }

            // Lowercase characters are 97 -> 122
            if (CharToCheck > 90 && CharToCheck < 97)
            {
                return false;
            }
            
            // Out of ASCII range for alphanumeric characters
            if (CharToCheck > 122)
            {
                return false;
            }

            return true;
        }

        // Check if a character is typable special character
        public static bool IsCharTypable(char CharToCheck)
        {
            return CharToCheck == ' ' || char.IsLetterOrDigit(CharToCheck) || SpecialCharacters.Contains(CharToCheck);
        }
    }
}
