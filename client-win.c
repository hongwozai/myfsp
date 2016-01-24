/**
 * @file   client-win.c
 * @brief  gui版本的客户端
 * @author luzeya
 * @date   2015-08-06
 */
#include "myfsp.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

enum {
    ICON_COLUMN = 0,
    NAME_COLUMN,
    SIZE_COLUMN,
    TYPE_COLUMN,
    N_COLUMNS
};

typedef struct {
    GtkWidget *w1;
    GtkWidget *w2;
    GtkWidget *self;
} Widget_2;

int sockfd, listenfd, conn;
bool isport;
char current_dir[1024];
char gbuf[1024];

GtkWidget *create_reg_window(GtkWidget*);
GtkWidget *create_login_window();
GtkWidget *create_main_window();
GtkWidget *create_main_toolbar();
GtkWidget *create_main_view();
void view_list_append(GtkListStore *list, int isdir, char *name, int size);
void view_list_data(GtkListStore *list);
void on_login_cancel_clicked(GtkButton *button, gpointer user_data);
GtkWidget *create_share_window();
GtkWidget *create_share_view();
GtkWidget *create_share_toolbar(GtkTreeView*);
GtkWidget *create_self_window();

void
view_list_get_name(GtkTreeView *treeview, char **name)
{
    GtkTreeIter iter;
    GtkTreeViewColumn *col;
    GtkTreePath *path;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(treeview);
    gtk_tree_view_get_cursor(treeview, &path, &col);
    if (!path) {
        debug("not select!");
        exit(1);
    }
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, NAME_COLUMN, name, -1);
}

void
on_login_ok_clicked(GtkButton *button, Widget_2* data)
{
    char ip[30];
    char tmp[30];
    int port;
    char buf[RCVBUFSIZE];
    GtkWidget *dialog;
    GtkWidget *main;

    /* dialog */
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "login failure!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    /* usr */
    sprintf(buf, REQUEST_NAME, gtk_entry_get_text(GTK_ENTRY(data->w1)));
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("buf:%s", buf);
    /* port */
    /* 创建一个服务器 */
    listenfd = anet_tcp_server("127.0.0.1", 0, 0);
    if (listenfd == -1) {
        err("Can't open port!");
        close(listenfd);
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    }
    getlocalsockaddr(listenfd, ip, &port);
    debug("ip:%s, port:%d", ip, port);
    sprintf(tmp, "%d", port);
    sprintf(buf, REQUEST_PORT, tmp);
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    if (buf[2] == '0') {
        debug("port success!");
    } else {
        debug("port not success!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    }
    /* passwd */
    sprintf(buf, REQUEST_PASS, gtk_entry_get_text(GTK_ENTRY(data->w2)));
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    if (buf[2] == '0') {
        debug("login success!");
        strcpy(current_dir, "/");
        main = create_main_window();
        gtk_widget_show_all(main);
        gtk_widget_hide(data->self);
        return;
    } else if (buf[2] == '2') {
        debug("login fail!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    } else {
        debug("shit!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    }
}

void
on_login_cancel_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *reg;

    reg = create_reg_window(user_data);
    gtk_widget_show_all(reg);
    gtk_widget_hide(user_data);
}

void
on_reg_cancel_clicked(GtkButton *button, Widget_2* user_data)
{
    gtk_widget_hide(user_data->w1);
    gtk_widget_show_all(user_data->w2);
    free(user_data);
}

void
on_reg_ok_clicked(GtkButton *button, Widget_2* user_data)
{
    char buf[1024];
    GtkWidget *dialog;

    /* dialog */
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "failure!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    /* register user */
    sprintf(buf, REQUEST_REG,
            gtk_entry_get_text(GTK_ENTRY(user_data->w1)),
            gtk_entry_get_text(GTK_ENTRY(user_data->w2)));
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    if (buf[2] == '1') {
        debug("reg failure!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    }
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    "success!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
}

GtkWidget *
create_reg_window(GtkWidget *login_window)
{
    GtkWidget *widget;
    GtkWidget *vbox;
    GtkWidget *hbox1, *hbox2, *hbox3, *hbox4;
    GtkWidget *usr_label, *pass_label, *again_label;
    GtkWidget *usr, *pass, *again;
    GtkWidget *ok, *cancel;

    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widget), "register");
    gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
    /* label */
    usr_label = gtk_label_new("name:");
    pass_label = gtk_label_new("pass:");
    again_label = gtk_label_new("again:");
    /* entry */
    usr = gtk_entry_new();
    pass = gtk_entry_new();
    again = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pass), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(again), FALSE);
    /* button */
    ok = gtk_button_new_with_label("ok");
    cancel = gtk_button_new_with_label("cancel");
    /* container */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), usr_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), usr, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), pass_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), pass, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), again_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), again, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox4), cancel, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox4), ok, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox3, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox4, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(widget), vbox);
    g_signal_connect(G_OBJECT(widget), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);
    Widget_2 *p = (Widget_2*)malloc(sizeof(Widget_2));
    p->w2 = login_window;
    p->w1 = widget;
    p->self = widget;
    g_signal_connect(G_OBJECT(cancel), "clicked",
                     G_CALLBACK(on_reg_cancel_clicked), p);
    Widget_2 *q = (Widget_2*)malloc(sizeof(Widget_2));
    q->w1 = usr;
    q->w2 = pass;
    p->self = widget;
    g_signal_connect(G_OBJECT(ok), "clicked",
                     G_CALLBACK(on_reg_ok_clicked), q);
    return widget;
}

GtkWidget *
create_login_window()
{
    GtkWidget *widget;
    GtkWidget *vbox;
    GtkWidget *hbox1, *hbox2, *hbox3;
    GtkWidget *usr_label, *pass_label;
    GtkWidget *usr, *pass;
    GtkWidget *ok, *cancel, *reg;

    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widget), "Login");
    gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
    /* label */
    usr_label = gtk_label_new("name:");
    pass_label = gtk_label_new("pass:");
    /* entry */
    usr = gtk_entry_new();
    pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pass), FALSE);
    /* button */
    ok = gtk_button_new_with_label("ok");
    cancel = gtk_button_new_with_label("cancel");
    reg = gtk_button_new_with_label("reg");
    /* container */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), usr_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), usr, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), pass_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), pass, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), reg, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), cancel, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), ok, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox3, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(widget), vbox);
    g_signal_connect(G_OBJECT(widget), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(cancel), "clicked",
                     G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(reg), "clicked",
                     G_CALLBACK(on_login_cancel_clicked), widget);
    Widget_2 *q = (Widget_2*)malloc(sizeof(Widget_2));
    q->w1 = usr;
    q->w2 = pass;
    q->self = widget;
    g_signal_connect(G_OBJECT(ok), "clicked",
                     G_CALLBACK(on_login_ok_clicked), q);
    return widget;
}

void
on_mkdir_button_clicked(GtkButton *button, Widget_2 *w)
{
    const char *p;
    char buf[1024];
    GtkTreeModel *list;

    p = gtk_entry_get_text(GTK_ENTRY(w->w1));
    strcpy(gbuf, p);
    gtk_widget_destroy(w->self);
    free(w);
    debug("%s", gbuf);
    sprintf(buf, REQUEST_MKDIR, gbuf);
    send_request(sockfd, 0, buf);
    recv_request(sockfd, 0, buf, sizeof(buf));
    debug("buf:%s", buf);
    list = gtk_tree_view_get_model((GtkTreeView*)w->w2);
    view_list_data(GTK_LIST_STORE(list));
}

void
on_toolbar_mkdir_clicked(GtkToolButton *toolbutton, GtkTreeView *treeview)
{
    GtkWidget *window;
    GtkWidget *box,*entry, *button;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Mkdir");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
    entry  = gtk_entry_new();
    button = gtk_button_new_with_label("ok");
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), box);
    Widget_2 *p = (Widget_2*)malloc(sizeof(Widget_2));
    p->w1 = entry;
    p->w2 = (GtkWidget*)treeview;
    p->self = window;
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(on_mkdir_button_clicked), p);
    gtk_widget_show_all(window);
}

void
on_toolbar_rm_clicked(GtkToolButton *toolbutton, GtkTreeView *treeview)
{
    GtkTreeModel *model;
    char *name;
    char buf[1024];

    model = gtk_tree_view_get_model(treeview);
    view_list_get_name(treeview, &name);
    debug("%s", name);
    sprintf(buf, REQUEST_RM, name);
    send_request(sockfd, 0, buf);
    recv_request(sockfd, 0, buf, sizeof(buf));
    debug("buf:%s", buf);
    view_list_data(GTK_LIST_STORE(model));
}

GtkWidget*
create_select_file(int isdir)
{
    GtkWidget *file;
    GtkFileChooserAction action;
    char *title;

    title = isdir?"Input File Name":"Select File";
    action = isdir?GTK_FILE_CHOOSER_ACTION_SAVE:GTK_FILE_CHOOSER_ACTION_OPEN;
    file = gtk_file_chooser_dialog_new(title, NULL,
                                       action,
                                       "cancel", GTK_RESPONSE_CANCEL,
                                       "ok", GTK_RESPONSE_ACCEPT,
                                       NULL);
    return file;
}

void
on_toolbar_upload_clicked(GtkToolButton *_button, GtkTreeView *treeview)
{
    bool status;
    char filename[128];
    char *name;
    char buf[1024];
    GtkWidget *dialog;
    GtkWidget *file;
    GtkTreeModel *model;

    file = create_select_file(0);
    if (GTK_RESPONSE_ACCEPT != gtk_dialog_run(GTK_DIALOG(file))) {
        gtk_widget_destroy(file);
        return;
    }
    name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file));
    debug("%s", name);
    get_filename(name, filename);
    debug("%s", filename);
    sprintf(buf, REQUEST_UP, filename);
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    /* 接受文件 */
    conn = anet_tcp_accept(listenfd, 0);
    if (conn == -1) {
        err("accept error!");
        goto error;
    }
    status = write_file(sockfd, conn, NULL, name);
    if (status == FALSE) {
        err("write file failure!");
        goto error;
    }
    close(conn);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    gtk_widget_destroy(file);
    model = gtk_tree_view_get_model(treeview);
    view_list_data(GTK_LIST_STORE(model));
    return;
error:
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    "upload error!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    return;
}

void
on_toolbar_down_clicked(GtkToolButton *_button, GtkTreeView *treeview)
{
    char *name;
    char *filename;
    char buf[1024];
    GtkWidget *dialog;
    GtkWidget *file;

    view_list_get_name(treeview, &filename);
    debug("%s", filename);
    file = create_select_file(1);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file), filename);
    if (GTK_RESPONSE_ACCEPT != gtk_dialog_run(GTK_DIALOG(file))) {
        gtk_widget_destroy(file);
        return;
    }
    name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file));
    debug("%s", name);
    /* get */
    sprintf(buf, REQUEST_GET, filename);
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    if (buf[2] != '0') {
        debug("error!");
        goto error;
    }
    conn = anet_tcp_accept(listenfd, 0);
    if (conn == -1) {
        err("accept error!");
        goto error;
    }
    read_file(sockfd, conn, name, NULL);
    close(conn);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    gtk_widget_destroy(file);
    return;
error:
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    "down error!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    return;
}

void
on_shareto_button_clicked(GtkButton *_button, Widget_2 *w)
{
    char buf[1024];
    GtkWidget *dialog;
    const char *usr;
    char *filename;

    usr = gtk_entry_get_text(GTK_ENTRY(w->w1));
    view_list_get_name((GtkTreeView*)(w->w2), &filename);
    debug("%s,%s", usr, filename);
    /* 共享给所有人用all, 第一个是共享给人， 第二个是共享的文件 */
    sprintf(buf, REQUEST_SHARE, usr, filename);
    send_request(sockfd, 0, buf);
    recv_request(sockfd, 0, buf, sizeof(buf));
    debug("buf:%s", buf);
    if (buf[2] == '1') {
        goto error;
    }
    debug("share success!");
    gtk_widget_destroy(w->self);
    return;
error:
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    "share error!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
}

void
on_toolbar_share_clicked(GtkToolButton *_button, GtkTreeView *treeview)
{
    char *filename;
    GtkWidget *window;
    GtkWidget *box,*entry, *button;

    view_list_get_name(treeview, &filename);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Share to");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
    entry  = gtk_entry_new();
    button = gtk_button_new_with_label("share");
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), box);
    Widget_2 *p = (Widget_2*)malloc(sizeof(Widget_2));
    p->w1 = entry;
    p->w2 = (GtkWidget*)treeview;
    p->self = window;
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(on_shareto_button_clicked), p);
    gtk_widget_show_all(window);
}

void
on_toolbar_cancel_clicked(GtkToolButton *_button, GtkTreeView *treeview)
{
    GtkWidget *slist;

    slist = create_self_window();
    gtk_widget_show_all(slist);
}

void
on_toolbar_slist_clicked(GtkToolButton *button, gpointer user_data)
{
    GtkWidget *slist;

    slist = create_share_window();
    gtk_widget_show_all(slist);
}

GtkWidget*
create_main_toolbar(GtkTreeView *treeview)
{
    GtkToolItem *upload, *down, *slist, *share, *cancel;
    GtkToolItem *mkdir, *rm;
    GtkWidget *toolbar;

    toolbar = gtk_toolbar_new();
    upload = gtk_tool_button_new(NULL, "upload");
    down = gtk_tool_button_new(NULL, "down");
    slist = gtk_tool_button_new(NULL, "slist");
    share = gtk_tool_button_new(NULL, "share");
    mkdir = gtk_tool_button_new(NULL, "mkdir");
    cancel = gtk_tool_button_new(NULL, "sharel");
    rm = gtk_tool_button_new(NULL, "rm");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), slist, 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), cancel, 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), share, 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), down, 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), upload, 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), rm, 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), mkdir, 0);
    g_signal_connect(G_OBJECT(mkdir), "clicked",
                     G_CALLBACK(on_toolbar_mkdir_clicked), treeview);
    g_signal_connect(G_OBJECT(rm), "clicked",
                     G_CALLBACK(on_toolbar_rm_clicked), treeview);
    g_signal_connect(G_OBJECT(upload), "clicked",
                     G_CALLBACK(on_toolbar_upload_clicked), treeview);
    g_signal_connect(G_OBJECT(down), "clicked",
                     G_CALLBACK(on_toolbar_down_clicked), treeview);
    g_signal_connect(G_OBJECT(share), "clicked",
                     G_CALLBACK(on_toolbar_share_clicked), treeview);
    g_signal_connect(G_OBJECT(slist), "clicked",
                     G_CALLBACK(on_toolbar_slist_clicked), NULL);
    g_signal_connect(G_OBJECT(cancel), "clicked",
                     G_CALLBACK(on_toolbar_cancel_clicked), treeview);
    return toolbar;
}

void
view_list_append(GtkListStore *list, int isdir, char *name,
                 int size)
{
    GError *error = NULL;
    GtkTreeIter iter;
    GdkPixbuf *folder, *file;
    folder = gdk_pixbuf_new_from_file("folder.svg", &error);
    if (!folder) {
        printf("shit:%s", error->message);
        exit(1);
    }
    file = gdk_pixbuf_new_from_file("empty.png", &error);
    if (!folder) {
        printf("shit:%s", error->message);
        exit(1);
    }
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, ICON_COLUMN, isdir?folder:file,
                       NAME_COLUMN, name,
                       SIZE_COLUMN, size,
                       TYPE_COLUMN, isdir?"folder":"file", -1);
}

void
view_list_data(GtkListStore *list)
{
    char buf[RCVBUFSIZE];
    GtkWidget *dialog;
    char isdirc;
    int  isdir;
    char filename[30];
    int size;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "list failure!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_list_store_clear(GTK_LIST_STORE(list));
    send_request(sockfd, 0, REQUEST_LIST);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    conn = anet_tcp_accept(listenfd, 0);
    if (conn == -1) {
        err("accept error!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    }
    while (recv_repl(conn, 0, buf, sizeof(buf)) != 0) {
        sscanf(buf, "%c %d %s", &isdirc, &size, filename);
        isdir = (isdirc == 'd')?1:0;
        debug("dir:%s,file:%c %d %s",
              current_dir, isdirc, size, filename);
        if (strcmp(filename, ".") == 0) {
            continue;
        }
        if (strcmp(current_dir, "/") == 0) {
            if (strcmp(filename, "..") == 0) {
                continue;
            }
        }
        view_list_append(list, isdir, filename, size);
    }
    close(conn);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
}

GtkListStore*
create_view_list()
{
    GtkListStore *list;
    /* list store */
    list = gtk_list_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                              G_TYPE_INT, G_TYPE_STRING);
    view_list_data(list);
    return list;
}

void
on_view_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                      GtkTreeViewColumn *column, gpointer user_data)
{
    GtkTreeModel *tree_model;
    GtkTreeIter iter;
    char *name;
    char *type;
    char buf[1024];

    tree_model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get_iter(tree_model, &iter, path);
    gtk_tree_model_get(tree_model, &iter,
                       NAME_COLUMN, &name,
                       TYPE_COLUMN, &type,
                       -1);
    debug("%s", name);
    if (strcmp(type, "folder") == 0) {
        /* cd request */
        sprintf(buf, REQUEST_CD, name);
        send_request(sockfd, 0, buf);
        recv_request(sockfd, 0, buf, sizeof(buf));
        debug("buf:%s", buf);
        if (strcmp(name, "..") == 0) {
            path_rm_dir(current_dir);
        } else {
            path_add_dir(current_dir, name);
        }
        view_list_data(GTK_LIST_STORE(tree_model));
    }
}

GtkWidget*
create_main_view()
{
    GtkWidget *view;
    GtkListStore *list;
    GtkTreeViewColumn *name, *size, *type;
    GtkCellRenderer *text, *icon;

    list = create_view_list();
    /* tree view */
    view = gtk_tree_view_new();
    /* column name*/
    text = gtk_cell_renderer_text_new();
    icon = gtk_cell_renderer_pixbuf_new();
    name = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(name, "Name");
    gtk_tree_view_column_pack_start(name, icon, FALSE);
    gtk_tree_view_column_pack_start(name, text, FALSE);
    gtk_tree_view_column_add_attribute(name, icon, "pixbuf", ICON_COLUMN);
    gtk_tree_view_column_add_attribute(name, text, "text", NAME_COLUMN);
    /* column size */
    size = gtk_tree_view_column_new_with_attributes("Size", text, "text", SIZE_COLUMN, NULL);
    type = gtk_tree_view_column_new_with_attributes("Type", text, "text", TYPE_COLUMN, NULL);
    gtk_tree_view_column_set_resizable(name, TRUE);
    gtk_tree_view_column_set_resizable(size, TRUE);
    gtk_tree_view_column_set_resizable(type, TRUE);
    /* column attribute */
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), name);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), size);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), type);
    /* data */
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(list));
    g_signal_connect(G_OBJECT(view), "row-activated",
                     G_CALLBACK(on_view_row_activated), NULL);
    return view;
}

GtkWidget*
create_main_window()
{
    GtkWidget *widget;
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *treeview;

    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(widget), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);
    gtk_window_set_title(GTK_WINDOW(widget), "File");
    gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(widget), 500, 300);
    /* treeview */
    treeview = create_main_view();
    /* toolbar */
    toolbar = create_main_toolbar( GTK_TREE_VIEW(treeview) );
    /* container */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), treeview, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(widget), vbox);
    return widget;
}

/* ==========================self window=========================== */
enum {
    SELF_ICON_COLUMN = 0,
    SELF_NAME_COLUMN,
    SELF_TO_COLUMN,
    SELF_N_COLUMNS
};
void
self_list_append(GtkListStore *list, char *name, int isdir,
                 char *tousr)
{
    GError *error = NULL;
    GtkTreeIter iter;
    GdkPixbuf *folder, *file;
    folder = gdk_pixbuf_new_from_file("folder.svg", &error);
    if (!folder) {
        printf("shit:%s", error->message);
        exit(1);
    }
    file = gdk_pixbuf_new_from_file("empty.png", &error);
    if (!folder) {
        printf("shit:%s", error->message);
        exit(1);
    }
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, ICON_COLUMN, isdir?folder:file,
                       SELF_NAME_COLUMN, name,
                       SELF_TO_COLUMN, tousr, -1);
}

void
self_list_data(GtkListStore *list)
{
    char buf[RCVBUFSIZE];
    GtkWidget *dialog;
    char isdirc;
    int  isdir;
    char filename[30];
    char tousr[30];

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "self share list failure!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_list_store_clear(GTK_LIST_STORE(list));
    send_request(sockfd, 0, REQUEST_SELF_SHARE_LIST);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    conn = anet_tcp_accept(listenfd, 0);
    if (conn == -1) {
        err("accept error!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    }
    while (recv_repl(conn, 0, buf, sizeof(buf)) != 0) {
        sscanf(buf, "%s %s %c", tousr, filename, &isdirc);
        isdir = (isdirc == 't')?1:0;
        debug("%s %s %c", tousr, filename, isdirc);
        self_list_append(list, filename, isdir, tousr);
    }
    close(conn);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
}

GtkListStore*
create_self_view_list()
{
    GtkListStore *list;
    /* list store */
    list = gtk_list_store_new(SELF_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                              G_TYPE_STRING);
    self_list_data(list);
    return list;
}

void
self_list_get(GtkTreeView *treeview, char **name, char **tousr)
{
    GtkTreeIter iter;
    GtkTreeViewColumn *col;
    GtkTreePath *path;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(treeview);
    gtk_tree_view_get_cursor(treeview, &path, &col);
    if (!path) {
        debug("not select!");
        exit(1);
    }
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, SELF_NAME_COLUMN, name
                       ,SELF_TO_COLUMN, tousr, -1);
}

void
on_self_cancel_clicked(GtkToolButton *button, GtkTreeView *treeview)
{
    char *filename;
    char *tousr;
    char buf[1024];
    GtkWidget *dialog;

    self_list_get(treeview, &filename, &tousr);
    debug("%s %s", filename, tousr);
    sprintf(buf, REQUEST_CANCAL, tousr, filename);
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("buf:%s", buf);
    if (buf[2] == '1') {
        goto error;
    }
    debug("cancel success!");
    self_list_data( GTK_LIST_STORE(gtk_tree_view_get_model(treeview)) );
    return;
error:
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    "cancal error!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    return;
}

GtkWidget*
create_self_toolbar(GtkTreeView *treeview)
{
    GtkToolItem *cancel;
    GtkWidget *toolbar;

    toolbar = gtk_toolbar_new();
    cancel = gtk_tool_button_new(NULL, "cancel");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), cancel, 0);
    g_signal_connect(G_OBJECT(cancel), "clicked",
                     G_CALLBACK(on_self_cancel_clicked), treeview);
    return toolbar;
}

GtkWidget*
create_self_view()
{
    GtkWidget *view;
    GtkListStore *list;
    GtkTreeViewColumn *tousr, *name;
    GtkCellRenderer *text, *icon;

    list = create_self_view_list();
    /* tree view */
    view = gtk_tree_view_new();
    /* column name*/
    text = gtk_cell_renderer_text_new();
    icon = gtk_cell_renderer_pixbuf_new();
    name = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(name, "Name");
    gtk_tree_view_column_pack_start(name, icon, FALSE);
    gtk_tree_view_column_pack_start(name, text, FALSE);
    gtk_tree_view_column_add_attribute(name, icon, "pixbuf", SELF_ICON_COLUMN);
    gtk_tree_view_column_add_attribute(name, text, "text", SELF_NAME_COLUMN);
    /* column size */
    tousr = gtk_tree_view_column_new_with_attributes("To", text, "text", SELF_TO_COLUMN, NULL);
    gtk_tree_view_column_set_resizable(name, TRUE);
    gtk_tree_view_column_set_resizable(tousr, TRUE);
    /* column attribute */
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), name);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), tousr);
    /* data */
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(list));
    return view;
}

GtkWidget*
create_self_window()
{
    GtkWidget *widget;
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *treeview;

    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widget), "Self Share");
    gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(widget), 300, 100);
    /* treeview */
    treeview = create_self_view();
    /* toolbar */
    toolbar = create_self_toolbar( GTK_TREE_VIEW(treeview) );
    /* container */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), treeview, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(widget), vbox);
    return widget;
}

/* ==========================share window=========================== */
enum {
    SHARE_ICON_COLUMN = 0,
    SHARE_NAME_COLUMN,
    SHARE_FROM_COLUMN,
    SHARE_TO_COLUMN,
    SHARE_N_COLUMNS
};
void
share_list_append(GtkListStore *list, char *name, int isdir, char *fromusr,
                  char *tousr)
{
    GError *error = NULL;
    GtkTreeIter iter;
    GdkPixbuf *folder, *file;
    folder = gdk_pixbuf_new_from_file("folder.svg", &error);
    if (!folder) {
        printf("shit:%s", error->message);
        exit(1);
    }
    file = gdk_pixbuf_new_from_file("empty.png", &error);
    if (!folder) {
        printf("shit:%s", error->message);
        exit(1);
    }
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, ICON_COLUMN, isdir?folder:file,
                       SHARE_NAME_COLUMN, name,
                       SHARE_FROM_COLUMN, fromusr,
                       SHARE_TO_COLUMN, tousr, -1);
}

void
share_list_data(GtkListStore *list)
{
    char buf[RCVBUFSIZE];
    GtkWidget *dialog;
    char isdirc;
    int  isdir;
    char filename[30];
    char fromusr[30];
    char tousr[30];

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "share list failure!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_list_store_clear(GTK_LIST_STORE(list));
    send_request(sockfd, 0, REQUEST_SHARE_LIST);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    conn = anet_tcp_accept(listenfd, 0);
    if (conn == -1) {
        err("accept error!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        return;
    }
    while (recv_repl(conn, 0, buf, sizeof(buf)) != 0) {
        sscanf(buf, "%s %s %s %c", fromusr, tousr, filename, &isdirc);
        isdir = (isdirc == 't')?1:0;
        debug("%s %s %s %c", fromusr, tousr, filename, isdirc);
        share_list_append(list, filename, isdir, fromusr, tousr);
    }
    close(conn);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
}

GtkListStore*
create_share_view_list()
{
    GtkListStore *list;
    /* list store */
    list = gtk_list_store_new(SHARE_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                              G_TYPE_STRING, G_TYPE_STRING);
    share_list_data(list);
    return list;
}

void
share_list_get(GtkTreeView *treeview, char **name, char **fromusr)
{
    GtkTreeIter iter;
    GtkTreeViewColumn *col;
    GtkTreePath *path;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(treeview);
    gtk_tree_view_get_cursor(treeview, &path, &col);
    if (!path) {
        debug("not select!");
        exit(1);
    }
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, SHARE_NAME_COLUMN, name
                       ,SHARE_FROM_COLUMN, fromusr, -1);
}

void
on_share_down_clicked(GtkToolButton *button, GtkTreeView *treeview)
{
    char *name;
    char *filename;
    char *fromusr;
    char buf[1024];
    GtkWidget *dialog;
    GtkWidget *file;

    share_list_get(treeview, &filename, &fromusr);
    debug("%s", filename);
    file = create_select_file(1);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file), filename);
    if (GTK_RESPONSE_ACCEPT != gtk_dialog_run(GTK_DIALOG(file))) {
        gtk_widget_destroy(file);
        return;
    }
    name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file));
    debug("%s", name);
    /* get */
    sprintf(buf, REQUEST_DOWN_SHARE, fromusr, filename);
    send_request(sockfd, 0, buf);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    if (buf[2] != '0') {
        debug("error!");
        goto error;
    }
    conn = anet_tcp_accept(listenfd, 0);
    if (conn == -1) {
        err("accept error!");
        goto error;
    }
    read_file(sockfd, conn, name, NULL);
    close(conn);
    recv_repl(sockfd, 0, buf, sizeof(buf));
    debug("%s", buf);
    gtk_widget_destroy(file);
    return;
error:
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    "down error!");
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    return;
}

GtkWidget*
create_share_toolbar(GtkTreeView *treeview)
{
    GtkToolItem *down;
    GtkWidget *toolbar;

    toolbar = gtk_toolbar_new();
    down = gtk_tool_button_new(NULL, "down");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), down, 0);
    g_signal_connect(G_OBJECT(down), "clicked",
                     G_CALLBACK(on_share_down_clicked), treeview);
    return toolbar;
}

GtkWidget*
create_share_view()
{
    GtkWidget *view;
    GtkListStore *list;
    GtkTreeViewColumn *fromusr, *tousr, *name;
    GtkCellRenderer *text, *icon;

    list = create_share_view_list();
    /* tree view */
    view = gtk_tree_view_new();
    /* column name*/
    text = gtk_cell_renderer_text_new();
    icon = gtk_cell_renderer_pixbuf_new();
    name = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(name, "Name");
    gtk_tree_view_column_pack_start(name, icon, FALSE);
    gtk_tree_view_column_pack_start(name, text, FALSE);
    gtk_tree_view_column_add_attribute(name, icon, "pixbuf", SHARE_ICON_COLUMN);
    gtk_tree_view_column_add_attribute(name, text, "text", SHARE_NAME_COLUMN);
    /* column size */
    fromusr = gtk_tree_view_column_new_with_attributes("From", text, "text", SHARE_FROM_COLUMN, NULL);
    tousr = gtk_tree_view_column_new_with_attributes("To", text, "text", SHARE_TO_COLUMN, NULL);
    gtk_tree_view_column_set_resizable(name, TRUE);
    gtk_tree_view_column_set_resizable(fromusr, TRUE);
    gtk_tree_view_column_set_resizable(tousr, TRUE);
    /* column attribute */
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), name);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), fromusr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), tousr);
    /* data */
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(list));
    return view;
}

GtkWidget*
create_share_window()
{
    GtkWidget *widget;
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *treeview;

    widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(widget), "Share");
    gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(widget), 300, 100);
    /* treeview */
    treeview = create_share_view();
    /* toolbar */
    toolbar = create_share_toolbar( GTK_TREE_VIEW(treeview) );
    /* container */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), treeview, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(widget), vbox);
    return widget;
}

int
main(int argc, char *argv[])
{
    bool status;
    GtkWidget *login;
    char ip[15];
    int port;
    /* cfg */
    mycfg_readstring("client.ini", "global", "ip", ip);
    mycfg_readint("client.ini", "global", "port", &port);
    /* start */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    sockfd = anet_tcp_client();
    if (sockfd == -1) {
        err("shit!");
        return 1;
    }
    status = anet_tcp_connect(sockfd, ip, port, 3);
    if (status == FALSE) {
        if (errno == ETIMEDOUT) {
            debug("connect timeout!");
            return 1;
        }
        err("why?");
        debug("connect fail!");
        return 1;
    }
    /* gui */
    gtk_init(&argc, &argv);
    login = create_login_window();
    gtk_widget_show_all(login);
    gtk_main();
    close(sockfd);
    return 0;
}
