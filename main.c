#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_BUFFER_SIZE 4096
#define COMMAND_BUFFER_SIZE 4096

// Reads the entire output of a command into a dynamically allocated string.
char *read_command_output(const char *command)
{
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("Failed to run command");
        return NULL;
    }

    size_t bufsize = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(bufsize);
    if (!buffer)
    {
        perror("Failed to allocate memory");
        pclose(fp);
        return NULL;
    }

    size_t pos = 0;
    int c;
    while ((c = fgetc(fp)) != EOF)
    {
        if (pos + 1 >= bufsize)
        {
            bufsize *= 2;
            char *newBuffer = realloc(buffer, bufsize);
            if (!newBuffer)
            {
                perror("Failed to reallocate memory");
                free(buffer);
                pclose(fp);
                return NULL;
            }
            buffer = newBuffer;
        }
        buffer[pos++] = (char)c;
    }
    buffer[pos] = '\0';

    pclose(fp);
    return buffer;
}

// Finds the given JSON field and returns a pointer to the start of its string value.
// It looks for the key pattern, finds the colon that follows it, then locates the first
// double quote after the colon. It returns a pointer to the first character of the value.
char *extract_json_string_value(const char *json, const char *field_name)
{
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", field_name);

    char *keyStart = strstr(json, pattern);
    if (!keyStart)
    {
        return NULL;
    }

    // Now, find the colon after the key.
    char *colon = strchr(keyStart, ':');
    if (!colon)
    {
        return NULL;
    }

    // Find the first double quote after the colon (skip any whitespace)
    char *quoteStart = strchr(colon, '\"');
    if (!quoteStart)
    {
        return NULL;
    }
    // Return pointer to the character immediately after the quote.
    return quoteStart + 1;
}

// Parses a JSON string value (naively) given a pointer to its first character.
// It stops at the first unescaped double quote.
char *parse_json_string(const char *start)
{
    size_t len = 0;
    const char *p = start;
    while (*p)
    {
        if (*p == '\"')
        {
            // Check if the quote is escaped.
            int backslashCount = 0;
            const char *q = p - 1;
            while (q >= start && *q == '\\')
            {
                backslashCount++;
                q--;
            }
            // If even number of backslashes, the quote is not escaped.
            if (backslashCount % 2 == 0)
            {
                break;
            }
        }
        len++;
        p++;
    }

    char *result = malloc(len + 1);
    if (!result)
    {
        perror("Failed to allocate memory for JSON value");
        return NULL;
    }
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

// Unescapes a JSON string by converting escape sequences (e.g. "\\n") into actual characters.
char *unescape_json_string(const char *input)
{
    size_t len = strlen(input);
    // The output will never be longer than the input.
    char *output = malloc(len + 1);
    if (!output)
    {
        perror("Failed to allocate memory for unescaped string");
        return NULL;
    }

    const char *in = input;
    char *out = output;

    while (*in)
    {
        if (*in == '\\' && *(in + 1))
        {
            in++; // skip the backslash
            switch (*in)
            {
            case 'n':
                *out++ = '\n';
                break;
            case 't':
                *out++ = '\t';
                break;
            case 'r':
                *out++ = '\r';
                break;
            case 'b':
                *out++ = '\b';
                break;
            case 'f':
                *out++ = '\f';
                break;
            case '\\':
                *out++ = '\\';
                break;
            case '\"':
                *out++ = '\"';
                break;
            case 'u':
            {
                // Basic handling for Unicode escape sequences: \uXXXX.
                // We'll convert it if it fits in a single char (i.e. code point < 128).
                char hex[5] = {0};
                if (isxdigit(*(in + 1)) && isxdigit(*(in + 2)) &&
                    isxdigit(*(in + 3)) && isxdigit(*(in + 4)))
                {
                    strncpy(hex, in + 1, 4);
                    unsigned int code;
                    sscanf(hex, "%x", &code);
                    if (code < 128)
                    {
                        *out++ = (char)code;
                    }
                    else
                    {
                        // For simplicity, if the code doesn't fit, output a placeholder.
                        *out++ = '?';
                    }
                    in += 4; // Skip the four hex digits.
                }
                else
                {
                    // If not a valid Unicode escape, just output 'u'.
                    *out++ = 'u';
                }
                break;
            }
            default:
                // If unknown escape, output it as-is.
                *out++ = *in;
                break;
            }
        }
        else
        {
            *out++ = *in;
        }
        in++;
    }
    *out = '\0';
    return output;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "---USAGE---\nArg1(Prompt): \"Your question here\"\nArg2(Model): \"deepseek-r1:8b / llama3.2:3b (DEFAULT) / etc\" \n");
        return EXIT_FAILURE;
    }

    const char *question = argv[1];
    const char *model = argv[2];

    if (!model)
    {
        model = "llama3.2:3b";
    }

    if (strcmp(question, "-h") == 0 || strcmp(question, "--help") == 0)
    {
        fprintf(stderr, "---USAGE---\nArg1(Prompt): \"Your question here\"\nArg2(Model): \"deepseek-r1:8b / llama3.2:3b (DEFAULT) / etc\"\n");
        return EXIT_FAILURE;
    }

    // Create the command string.
    // Note: The JSON payload includes the question. We assume here that the question
    // is simple and does not need additional JSON escaping.
    char command[COMMAND_BUFFER_SIZE];
    int n = snprintf(command, sizeof(command),
                     "curl \"http://localhost:11434/api/chat\" -s -d \"{\\\"model\\\": \\\"%s\\\","
                     "\\\"messages\\\": [{\\\"role\\\": \\\"user\\\", \\\"content\\\": \\\"Write a very short awnser to the following question: %s\\\"}],"
                     "\\\"stream\\\": false}\"",
                     model, question);
    if (n < 0 || n >= sizeof(command))
    {
        fprintf(stderr, "Error: Command string too long.\n");
        return EXIT_FAILURE;
    }

    // Read the entire output of the command.
    char *json_response = read_command_output(command);
    if (!json_response)
    {
        return EXIT_FAILURE;
    }

    // Uncomment the next line to debug the full JSON response:
    // printf("Full JSON response:\n%s\n", json_response);

    // Locate the "message" object in the JSON response.
    char *message_ptr = strstr(json_response, "\"message\":");
    if (!message_ptr)
    {
        fprintf(stderr, "Could not find \"message\" in the response.\n");
        free(json_response);
        return EXIT_FAILURE;
    }

    // Within the "message" object, look for the "content" field.
    char *content_start = extract_json_string_value(message_ptr, "content");
    if (!content_start)
    {
        fprintf(stderr, "Could not find \"content\" in the \"message\" object.\n");
        free(json_response);
        return EXIT_FAILURE;
    }

    // Extract the string value (up to the closing unescaped quote).
    char *content_value = parse_json_string(content_start);
    if (!content_value)
    {
        fprintf(stderr, "Failed to extract the content string.\n");
        free(json_response);
        return EXIT_FAILURE;
    }

    // Unescape the JSON string to convert \n and other escape sequences to actual characters.
    char *formatted_output = unescape_json_string(content_value);
    if (!formatted_output)
    {
        free(content_value);
        free(json_response);
        return EXIT_FAILURE;
    }

    // Print the unescaped (formatted) message content.
    printf("%s\n", formatted_output);

    free(formatted_output);
    free(content_value);
    free(json_response);
    return EXIT_SUCCESS;
}