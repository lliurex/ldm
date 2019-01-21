#define PORT 3389

typedef struct rdp_info {
    gchar *username;
    gchar *password;
    gchar *domain;
    gchar *server;
    gchar *lang;
    gchar *rdpoptions;
    gint rdpfd;
    gint rdpslavefd;
    GPid rdppid;
} RdpInfo;

void __attribute__((constructor)) initialize();

/* Member functions */
void _get_domain();

void auth_rdesktop(void);
void close_rdesktop(void);
void init_rdesktop(void);
void rdesktop_session(void);
void start_rdesktop(void);

extern volatile sig_atomic_t child_exited;
