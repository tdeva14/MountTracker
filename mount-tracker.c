#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>

#define MAX_POLL_TIMEOUT 3600000 //1hr
#define MIN_POLL_TIMEOUT 900000 //15min

#define PROC_MOUNTS "/proc/self/mounts"
#define PROC_MOUNTINFO "/proc/self/mountinfo"

typedef struct mountInfo {
	int id;
	int parent_id;
	char *major_minor_pair;
	char *root;
	char *mount_point;
	char *mount_opts;
	char *opt_fields;
	char *seperator;
	char *fs_type;
	char *mount_dev;
	char *super_opts;
} mount_info;

// This is an array of device names to monitor
static GArray *dev_mon_list = NULL;
// These are the previous and current lists of mount_info structs
static GSList *prev_mount_list = NULL, *curr_mount_list = NULL;

// This function prints the usage instructions for the mount-tracker application
void print_usage()
{
	printf("Usage: mount-tracker -d <device> -t <timeout>\n");
	printf("-d: device to monitor. Multiple devices can be passed. Mandatory option\n");
	printf("-t: Optional. Configure timeout (millisecs) used by poll system call\n");
	printf("\n");
	printf("Example: mount-tracker -d mmcblk -d ubi\n");
	printf("Note: Make sure the devices passed in -d are already mounted before running the app\n");
	exit(-1);
}


// This function returns a GString containing the mount information for the specified devices
GString* snap_mounts()
{
	FILE *fptr = NULL;
	char buff[1024] = {0};

	GString* m_mounts = g_string_new("");

	fptr = fopen(PROC_MOUNTINFO,"r");
	if (fptr) {
		while (fgets(buff, sizeof(buff), fptr)) {
			for (int i = 0; i < dev_mon_list->len; i++) {
				if (NULL != strstr(buff, g_array_index(dev_mon_list, char*, i)))
					g_string_append(m_mounts, buff);
			}
			memset(buff, 0, sizeof(buff));
		}
		fclose(fptr);
	} else {
		/* File open failed. */
		printf("Error in opening %s\n", PROC_MOUNTS);
		exit(-1);
	}
	
	if (m_mounts->len)
		printf("Snapped mounts:\n%s\n", m_mounts->str);
	
	return m_mounts;
}

// This function saves the mount information to a GSList of mount_info structs
int save_mountinfo_list(GString *snapped_mounts, gboolean trigger)
{
	char* mounts_str = g_strdup(snapped_mounts->str);
	g_string_free(snapped_mounts, TRUE);

	char newline[] = "\n";
	char space[] = " ";

	char *out_str = NULL, *in_str = NULL;
	char *saveptr1 = NULL, *saveptr2 = NULL;

	mount_info *t_mount_info = NULL;

	out_str = strtok_r(mounts_str, newline, &saveptr1);

	while (out_str != NULL) {
		t_mount_info = (mount_info *) malloc (sizeof(mount_info));

		in_str = strtok_r(out_str, space, &saveptr2);

		for (int n = 1; in_str != NULL; n++) {
			if (n == 1) {
				t_mount_info->id = atoi(in_str);
			} else if (n == 2) {
				t_mount_info->parent_id = atoi(in_str);
			} else if (n == 3) {
				t_mount_info->major_minor_pair = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->major_minor_pair, in_str);
			} else if (n == 4) {
				t_mount_info->root = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->root, in_str);
			} else if (n == 5) {
				t_mount_info->mount_point = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->mount_point, in_str);
			} else if (n == 6) {
				t_mount_info->mount_opts = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->mount_opts, in_str);
			} else if (n == 7) {
				t_mount_info->opt_fields = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->opt_fields, in_str);
			} else if (n == 8) {
				t_mount_info->seperator = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->seperator, in_str);
			} else if (n == 9) {
				t_mount_info->fs_type = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->fs_type, in_str);
			} else if (n == 10) {
				t_mount_info->mount_dev = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->mount_dev, in_str);
			} else if (n == 11) {
				t_mount_info->super_opts = (char*) calloc(strlen(in_str)+1, sizeof(char));
				strcpy(t_mount_info->super_opts, in_str);
			} else {
				//ignore
			}

			in_str = strtok_r(NULL, space, &saveptr2);
		}

		if (trigger) 
			curr_mount_list = g_slist_append(curr_mount_list, t_mount_info);
		else 
			prev_mount_list = g_slist_append(prev_mount_list, t_mount_info);

		t_mount_info = NULL;
		out_str = strtok_r(NULL, newline, &saveptr1);
	}

	g_free(mounts_str);
}


void free_mount_info(gpointer data)
{
	mount_info *f_mount_info = (mount_info *) data;

	if (f_mount_info->major_minor_pair) {
		free(f_mount_info->major_minor_pair);
		f_mount_info->major_minor_pair = NULL;
	}

	if (f_mount_info->root) {
		free(f_mount_info->root);
		f_mount_info->root = NULL;
	}

	if (f_mount_info->mount_point) {
		free(f_mount_info->mount_point);
		f_mount_info->mount_point = NULL;
	}

	if (f_mount_info->mount_opts) {
		free(f_mount_info->mount_opts);
		f_mount_info->mount_opts = NULL;
	}

	if (f_mount_info->opt_fields) {
		free(f_mount_info->opt_fields);
		f_mount_info->opt_fields = NULL;
	}

	if (f_mount_info->seperator) {
		free(f_mount_info->seperator);
		f_mount_info->seperator = NULL;
	}

	if (f_mount_info->fs_type) {
		free(f_mount_info->fs_type);
		f_mount_info->fs_type = NULL;
	}

	if (f_mount_info->mount_dev) {
		free(f_mount_info->mount_dev);
		f_mount_info->mount_dev = NULL;
	}

	if (f_mount_info->super_opts) {
		free(f_mount_info->super_opts);
		f_mount_info->super_opts = NULL;
	}

	if (f_mount_info) {
		free(f_mount_info);
		f_mount_info = NULL;
	}
}


void cmp_mnt_fields_and_notify(const mount_info *prev_mount_info, const mount_info *curr_mount_info)
{
	/* We are concerned only about mount options of the mounted devices */
	char *device = prev_mount_info->mount_dev;
	char *p_mount_opts = prev_mount_info->mount_opts;
	char *c_mount_opts = curr_mount_info->mount_opts;
	
	if (0 != g_strcmp0(p_mount_opts, c_mount_opts))
		printf("Mount options for %s device changed from %s to %s\n", device, p_mount_opts, c_mount_opts);
}


int find_match(gconstpointer curr_list_data, gconstpointer prev_list_data)
{
	mount_info *c_mount_info = (mount_info *) curr_list_data;
	mount_info *p_mount_info = (mount_info *) prev_list_data;

	return g_strcmp0(p_mount_info->mount_dev, c_mount_info->mount_dev);
}


void process_list_element(gpointer element_data, gpointer user_data)
{
	mount_info *mnt_info = (mount_info *) element_data;
	GSList *list_to_match = curr_mount_list;

	GSList *ret_element = g_slist_find_custom(list_to_match, element_data, find_match);
	if (ret_element) {
		mount_info *r_mnt_info = (mount_info *) ret_element->data;
		cmp_mnt_fields_and_notify(mnt_info, r_mnt_info);
	} else {
		printf("%s device is un-mounted\n", mnt_info->mount_dev);
	}
}


void compare_snaps()
{
	/* Track mount changes & unmounts for snapped devices */
	g_slist_foreach(prev_mount_list, process_list_element, NULL);

	g_slist_free_full(prev_mount_list, free_mount_info);
	prev_mount_list = curr_mount_list;
	curr_mount_list = NULL;
}


void monitor_mounts(int poll_timeout)
{
	int mfd = open(PROC_MOUNTS, O_RDONLY, 0);

	struct pollfd pfds = {mfd, POLLERR|POLLPRI|POLLNVAL, 0};

	while (poll(&pfds, 1, poll_timeout) > -1) {
		if (pfds.revents & POLLPRI) {
			printf("\nReceived trigger for mount change\n\n");
			save_mountinfo_list(snap_mounts(), TRUE);
			compare_snaps();
		}
		pfds.revents = 0;
	}

	close(mfd);
}


int main(int argc, char **argv)
{
	int option = 0;
	int m_poll_timeout = MAX_POLL_TIMEOUT;
	opterr = 0;

	dev_mon_list = g_array_new(TRUE, TRUE, sizeof(char*));

	while ( -1 != (option = getopt(argc, argv, "d:t:")) ) {
		switch(option) {
			case 'd':
				g_array_append_val(dev_mon_list, optarg);
				break;

			case 't':
				m_poll_timeout = atoi(optarg);

				if (m_poll_timeout < MIN_POLL_TIMEOUT)
					m_poll_timeout =  MIN_POLL_TIMEOUT;
				else if (m_poll_timeout > MAX_POLL_TIMEOUT)
					m_poll_timeout = MAX_POLL_TIMEOUT;

				break;

			case '?':
				if ('d' == optopt)
					printf("Option '-%c' requires an argument\n", optopt);
				else
					printf("Unknown option '-%c'\n", optopt);

				print_usage();

				break;
		}
	}

	if (dev_mon_list->len < 1) {
		g_array_free(dev_mon_list, TRUE);
		print_usage();
	}
	
	// save initial snapshot of the desired mount devices
	save_mountinfo_list(snap_mounts(), FALSE);

	// start monitoring the mounts
	monitor_mounts(m_poll_timeout);

	// exit app gracefully
	g_array_free(dev_mon_list, TRUE);
	g_slist_free_full(prev_mount_list, free_mount_info);
	g_slist_free_full(curr_mount_list, free_mount_info);
	
	return 0;
}
