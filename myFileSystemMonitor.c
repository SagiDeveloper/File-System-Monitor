#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <libcli.h>

// DEFAULT PORT VALUE
#define PORT 8888

// CHANGE THE VALUE FOR LIMIT THE LOGS INSIDE THE APACHE SERVER  
#define HTML_DATA_LIMIT 1000

// Globals
int flag = 0;
struct cli_def *cli;

// Backtrace Command for libcli
int cmd_backtrace(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    cli_print(cli, "called %s with %s\r\n", __FUNCTION__, command);
    flag = 1;
    return CLI_OK;
}
void *my_backtrace(void *arg)
{
    struct cli_def *cli = (struct cli_def *)arg;

    int j, nptrs;
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, 100);
    cli_print(cli, "backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
              would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL)
    {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        cli_print(cli, "%s\n", strings[j]);

    free(strings);
    return NULL;
}

// String sub-functions
char **str_splitter(char *str, size_t *size)
{
    // split string to array of strings, seperator is ' '

    char **array = NULL;
    char *p;
    size_t items = 0, q;
    const char *sepa = " ";

    p = str;
    for (;;)
    {
        p += strspn(p, sepa);
        if (!(q = strcspn(p, sepa)))
            break;
        if (q)
        {
            array = realloc(array, (items + 1) * sizeof(char *));
            if (array == NULL)
                exit(EXIT_FAILURE);

            array[items] = malloc(q + 1);
            if (array[items] == NULL)
                exit(EXIT_FAILURE);

            strncpy(array[items], p, q);
            array[items][q] = 0;
            items++;
            p += q;
        }
    }
    *size = items;
    return array;
}
void str_concat(char **s1, const char *s2)
{
    //add two strings together

    int s1_len = strlen(*s1);
    int s2_len = strlen(s2);
    int len = s1_len + s2_len + 1;
    char *new_str = (char *)realloc(*s1, len); // +1 for the null-terminator
    if (new_str != NULL)
    {
        for (int i = 0; i < s2_len; i++)
        {
            new_str[s1_len++] = s2[i];
        }
        new_str[s1_len] = '\0';
        *s1 = new_str;
    }
    else
        exit(EXIT_FAILURE);
}
char *createCommand(char *html_data)
{
    //create command that make system call of html page

    char start[] = "<!DOCTYPE html><html lang='en'>  <head>    <meta charset='UTF-8' />    <meta name='viewport' content='width=device-width, initial-scale=1.0' />    <meta http-equiv='refresh' content='10' />    <title>File System Monitor</title>     <style>      * {        margin: 0;        padding: 0;        box-sizing: border-box;      }      header,      main,      footer,      nav,      div {        display: block;      }      body {        margin: 0;        background: black;      }      #wrapper{        padding: 5vh 5vw;      }      ul {        font-family: monospace;        font-weight: bold;        font-size: 3.5vh;        margin: 0 0 5vh 0;        padding: 0;        line-height: 1;        color: limegreen;        text-shadow: 0px 0px 10px limegreen;        list-style-type:none;      }      #message {        position: fixed;        font-family: monospace;        font-weight: bold;        text-transform: uppercase;        font-size: 4vh;        background: red;        box-shadow: 0 0 30px red;        text-shadow: 0 0 20px white;        color: white;        width: 20vw;        height: 15vh;        top: 50%;        left: 50%;        margin-right: -50%;        transform: translate(-50%, -50%);        text-align: center;        min-width: 200px;        animation-name: blink;        animation-duration: 3.0s;        animation-iteration-count: infinite;        animation-direction: alternate;        animation-timing-function: linear;      }      @keyframes blink {        0% {          opacity: 0;        }        100% {          opacity: 1;        }      }      p{        font-size: 2.3vh;         position: relative;        top:50%;        left: 50%;        transform: translate(-50%, -50%);      }      footer {        background-color: rgb(0, 0, 0);        position: fixed;        box-shadow: 0px 0px 5px limegreen;        border-top: 1px solid limegreen;        width: 100%;        bottom: 0px;        padding:5px 0px 5px 5px;        color: limegreen;        font-family: monospace;        font-weight: bold;        font-size: 3vh; }    </style>      </head>  <body>      <div id='wrapper'>        <div id='message'>            <p>Scanning...<br>Fetching more data in <span id='countdowntimer'>10 </span> Seconds</p>        </div>        <div id='console'> ";
    char end[] = "</div>    </div>    <footer></footer>        <script type='text/javascript'>        var timeleft = 10;        var downloadTimer = setInterval(function(){        timeleft--;        document.getElementById('countdowntimer').textContent = timeleft;        if(timeleft <= 0)            clearInterval(downloadTimer);        },1000);    </script></body></html>";

    int data_len = strlen(html_data);
    int fsize = data_len + strlen(start) + strlen(end) + 2 + 5 + 27 + 1;

    // Allocate dynamic memory and implement the data
    char *copy = malloc(fsize);
    if (copy != NULL)
    {
        size_t i = 0;
        char *ptr = NULL;

        copy[i++] = 'e';
        copy[i++] = 'c';
        copy[i++] = 'h';
        copy[i++] = 'o';
        copy[i++] = ' ';
        copy[i++] = '"';

        ptr = start;
        while (*ptr)
        {
            copy[i++] = *ptr;
            ptr++;
        }

        ptr = html_data;
        while (*ptr)
        {
            copy[i++] = *ptr;
            ptr++;
        }

        ptr = end;
        while (*ptr)
        {
            copy[i++] = *ptr;
            ptr++;
        }
        copy[i++] = '"';
        copy[i++] = ' ';
        copy[i++] = '>';
        copy[i++] = ' ';
        copy[i++] = '/';
        copy[i++] = 'v';
        copy[i++] = 'a';
        copy[i++] = 'r';
        copy[i++] = '/';
        copy[i++] = 'w';
        copy[i++] = 'w';
        copy[i++] = 'w';
        copy[i++] = '/';
        copy[i++] = 'h';
        copy[i++] = 't';
        copy[i++] = 'm';
        copy[i++] = 'l';
        copy[i++] = '/';
        copy[i++] = 'i';
        copy[i++] = 'n';
        copy[i++] = 'd';
        copy[i++] = 'e';
        copy[i++] = 'x';
        copy[i++] = '.';
        copy[i++] = 'h';
        copy[i++] = 't';
        copy[i++] = 'm';
        copy[i++] = 'l';
        copy[i] = '\0';
        return copy;
    }
    else
        exit(EXIT_FAILURE);
}

// Profiler
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{ 
}
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    if (flag)
    {
        flag = 0;
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, my_backtrace, cli);
        pthread_join(thread_id, NULL);
    }
}

// libcli (server)
void *my_libcli(void *arg)
{
    char *address = (char *)arg;
    struct sockaddr_in servaddr;
    int on = 1, x, s;

    // Must be called first to setup data structures
    cli = cli_init();

    // Set the hostname (shown in the the prompt)
    cli_set_hostname(cli, "FileSystemMonitor");

    // Set up a few simple one-level commands
    cli_register_command(cli, NULL, "backtrace", cmd_backtrace, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

    // Create a socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // Listen on port 8888
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(address);

    bind(s, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // Wait for a connection
    listen(s, 50);

    while ((x = accept(s, NULL, 0)))
    {
        // Pass the connection off to libcli
        cli_loop(cli, x);
        close(x);
    }

    return NULL;
}

// Notify events (client)
static void handle_events(int fd, int *wd, int argc, char *argv[], char **html_data, int *html_data_cnt, int *clientSocket, struct sockaddr_in *serverAddr)
{

    /* Some systems cannot read integer variables if they are not
	   properly aligned. On other systems, incorrect alignment may
	   decrease performance. Hence, the buffer used for reading from
	   the inotify file descriptor should have the same alignment as
	   struct inotify_event. */

    /* Current time */
    char date_time[26];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    int i;
    ssize_t len;
    char *ptr;

    /* Loop while events can be read from inotify file descriptor. */

    for (;;)
    {
        /* Read some events. */

        len = read(fd, buf, sizeof buf);
        if (len == -1 && errno != EAGAIN)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* If the nonblocking read() found no events to read, then
		   it returns -1 with errno set to EAGAIN. In that case,
		   we exit the loop. */

        if (len <= 0)
            break;

        /* Loop over all events in the buffer */
        for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
        {

            event = (const struct inotify_event *)ptr;

            // Skip is open
            if (event->mask & IN_OPEN)
                continue;

            // Skip DIR
            if (event->mask & IN_ISDIR)
                continue;

            // Reset the html data
            if (*html_data_cnt == HTML_DATA_LIMIT)
            {
                *html_data_cnt = 0;
                free(*html_data);
                *html_data = malloc((strlen(" ") + 1) * sizeof(char));
                if (*html_data == NULL)
                    exit(EXIT_FAILURE);
                strcpy(*html_data, " ");
            }

            /* Print the name of the file */
            char *udp_data = malloc((strlen("FILE ACCESSED: ") + 1) * sizeof(char));
            if (udp_data == NULL)
                exit(EXIT_FAILURE);

            strcpy(udp_data, "FILE ACCESSED: ");

            str_concat(&*html_data, "<ul> <li>FILE ACCESSED: ");

            for (i = 0; i < argc; ++i)
            {
                if (wd[i] == event->wd)
                {
                    str_concat(&udp_data, argv[i]);
                    str_concat(&udp_data, "/");

                    str_concat(&*html_data, argv[i]);
                    str_concat(&*html_data, "/");

                    break;
                }
            }

            /* Print the name of the file */

            if (event->len)
            {
                str_concat(&udp_data, event->name);
                str_concat(&udp_data, "\n");

                str_concat(&*html_data, event->name);
                str_concat(&*html_data, "</li>");
            }

            str_concat(&udp_data, "ACCESS: ");

            str_concat(&*html_data, "<li>ACCESS: ");

            /* Print event type */

            if (event->mask & IN_CLOSE_NOWRITE)
            {
                str_concat(&udp_data, "NO_WRITE\n");
                str_concat(&*html_data, "NO_WRITE</li>");
            }
            else if (event->mask & IN_CLOSE_WRITE)
            {
                str_concat(&udp_data, "WRITE\n");
                str_concat(&*html_data, "WRITE</li>");
            }

            /* Current Time */

            strftime(date_time, 26, "%d-%m-%Y %H:%M:%S\n\n", tm);

            str_concat(&udp_data, "TIME OF ACCESS: ");
            str_concat(&udp_data, date_time);

            sendto(*clientSocket, udp_data, strlen(udp_data),
                   MSG_CONFIRM, (const struct sockaddr *)&*serverAddr,
                   sizeof(*serverAddr));

            free(udp_data);

            str_concat(&*html_data, "<li>TIME OF ACCESS: ");
            str_concat(&*html_data, date_time);
            str_concat(&*html_data, "</li> </ul>");

            // Update html data count
            ++*html_data_cnt;
        }
        char *command = createCommand(*html_data);
        system(command);
        free(command);

    }
}
void inotify(int argc, char **argv, char *address)
{
    /* Read all available inotify events from the file descriptor 'fd'.
          wd is the table of watch descriptors for the directories in argv.
          argc is the length of wd and argv.
          argv is the list of watched directories.
          Entry 0 of wd and argv is unused. */

    char buf;
    int fd, i, poll_num;
    int *wd;
    nfds_t nfds;
    struct pollfd fds[2];

    printf("Press ENTER key to terminate.\n");

    /* Create the file descriptor for accessing the inotify API */

    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1)
    {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }

    /* Allocate memory for watch descriptors */

    wd = (int *)calloc(argc + 1, sizeof(int));
    if (wd == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    /* Mark directories for events
	   - file was opened
	   - file was closed */

    for (i = 0; i < argc; i++)
    {
        wd[i] = inotify_add_watch(fd, argv[i], IN_OPEN | IN_CLOSE);
        if (wd[i] == -1)
        {
            fprintf(stderr, "Cannot watch '%s'\n", argv[i]);
            perror("inotify_add_watch");
            exit(EXIT_FAILURE);
        }
    }

    /* Prepare for polling */

    nfds = 2;

    /* Console input */

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    /* Inotify input */

    fds[1].fd = fd;
    fds[1].events = POLLIN;

    /* Server */

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, my_libcli, address);

    /* Client */

    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (clientSocket < 0)
    {
        perror("Error in connection\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr = {'\0'};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(address);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error in connection\n\n");
        exit(EXIT_FAILURE);
    }

    /* Wait for events and/or terminal input */

    printf("Listening for events.\n");

    char *html_data = malloc((strlen(" ") + 1) * sizeof(char));
    if (html_data == NULL)
        exit(EXIT_FAILURE);
    strcpy(html_data, " ");
    int html_data_cnt = 0;

    while (1)
    {
        poll_num = poll(fds, nfds, -1);
        if (poll_num == -1)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (poll_num > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                /* Console input is available. Empty stdin and quit */

                while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
                    continue;
                break;
            }

            if (fds[1].revents & POLLIN)
            {
                /* Inotify events are available */

                handle_events(fd, wd, argc, argv, &html_data, &html_data_cnt, &clientSocket, &serverAddr);
            }
        }
    }

    printf("Listening for events stopped.\n");

    /* free splitted directories */
    for (int i = 0; i < argc; i++)
    {
        free(argv[i]);
    }
    free(argv);

    /* Close inotify file descriptor */
    close(fd);
    free(wd);

    /* Free data structures */
    cli_done(cli);

    /* Close client socket */
    close(clientSocket);
    free(html_data);
}

int main(int argc, char **argv)
{
    if (argc == 5)
    {
        int option_index = 0;
        char *directory = NULL;
        char *address = NULL;
        while ((option_index = getopt(argc, argv, "d:i:")) != -1)
        {
            switch (option_index)
            {
            case 'd':
                directory = optarg;
                break;
            case 'i':
                address = optarg;
                break;
            }
        }

        if (directory != NULL && address != NULL)
        {
            size_t cnt = 0;
            char **directories = str_splitter(directory, &cnt);
            inotify((int)cnt, directories, address);
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        printf("Usage: %s -d PATH -i IP\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}