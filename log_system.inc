#if defined _log_system_included
    #endinput 
#endif
#define _log_system_included


#if !defined mysql_included 
    #error "<log_system> requires <a_mysql> to correct work! You can obtain it here: https://github.com/pBlueG/SA-MP-MySQL/releases" 
#endif

#if !defined DB_LOGS_ACCOUNTS
    #error "DB_LOGS_ACCOUNTS not defined!"
#endif

#if !defined DB_LOGS_ACCOUNT_NAME
    #error "DB_LOGS_ACCOUNT_NAME not defined!"
#endif

#if !defined DB_LOGS_ACCOUNT_ID
    #error "DB_LOGS_ACCOUNT_ID not defined!"
#endif

#if defined LOG_TELEGRAM_BOT
    #if !defined _tgconnector_included
        #error "Please included <tgconnector> for Telegram support. You can obtain it here: https://github.com/Sreyas-Sreelal/tgconnector/releases"
    #endif

    #if !defined LOG_TELEGRAM_BOT_TOKEN
        #error "Please define the Telegram bot token before include <log_system>"
    #endif

    #if !defined LOG_TELEGRAM_CHAT_ID
        #error "Please define Telegram chat ID before include <log_system>"
    #endif
#endif

/*
    definitions
*/

#define LOG_CURRENT_VERSION 1.0 

#if !defined DB_LOGS
    #define DB_LOGS "logs"
#endif

#if !defined DIALOG_FIND_LOGS
    #define DIALOG_FIND_LOGS 4200
#endif

#if !defined DIALOG_LOAD_LOGS_PAGE
    #define DIALOG_LOAD_LOGS_PAGE 4201
#endif

#if !defined DIALOG_LOAD_LOGS_DETAIL
    #define DIALOG_LOAD_LOGS_DETAIL 4202
#endif

#if !defined MAX_LOG_TYPE_LENGHT
    #define MAX_LOG_TYPE_LENGHT 14
#endif

#if !defined MAX_LOG_MESSAGE_LENGHT
    #define MAX_LOG_MESSAGE_LENGHT 128
#endif

#if !defined MAX_LOGS_ON_PAGE
    #define MAX_LOGS_ON_PAGE 20
#endif

/*
    enums
*/

enum E_LOG_TYPE 
{
    LOG_HOUSE_TYPE,
    LOG_BUSINESS_TYPE,
    LOG_FACTION_TYPE,
    LOG_MONEY_TYPE,
    LOG_ADMIN_TYPE
};

enum E_PLAYER_TEMP_LOG_INFO
{
    E_PLAYER_TEMP_LOG_NAME[MAX_PLAYER_NAME + 1],
    E_PLAYER_TEMP_LOG_ID,
    E_PLAYER_TEMP_LOG_PAGE[MAX_LOGS_ON_PAGE]
};

/*
    vars
*/

static 
    MySQL: g_log_sql_handle,
#if defined LOG_TELEGRAM_BOT
    TGBot:g_log_telegram_bot,
#endif
    g_player_temp_log_data[MAX_PLAYERS][E_PLAYER_TEMP_LOG_INFO],
    pVar__LogsPageID[] = "pVar__LogsPageID";

static const NULL_g_player_log_data[E_PLAYER_TEMP_LOG_INFO] = 
{
    EOS,
    0,
    0
};

/*  
    forwards
*/

forward OnPlayerLoadAccountID(playerid);
forward OnPlayerLogsSearch(playerid, page);
forward OnPlayerLoadLogsDetails(playerid);
/*  
    functions
*/

stock Log_GetMySQLConnectionHandle()
{
    return g_log_sql_handle;
}

stock Log_SetMySQLConnectionHandle(MySQL: handle)
{
    g_log_sql_handle = handle;
}

stock Log_Init()
{
    new 
        Cache: cache_id = mysql_query(g_log_sql_handle, !"SHOW TABLES LIKE '"DB_LOGS"'", true),
        current_tick = GetTickCount();

    if (!cache_num_rows()) {
        mysql_query(g_log_sql_handle,
            "\
                CREATE TABLE `"DB_LOGS"` ( \
                    `id` int(11) NOT NULL AUTO_INCREMENT, \
                    `type` int(11) NOT NULL, \
                    `name` varchar(24) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci DEFAULT NULL, \
                    `account_id` int(11) NOT NULL, \
                    `log` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci DEFAULT NULL, \
                    `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP, \
                    PRIMARY KEY (`id`) \
                )   ENGINE=INNODB DEFAULT \
                    CHARSET=utf8mb4, \
                    COLLATE utf8mb4_general_ci; \
            ", false
       );
        mysql_query(g_log_sql_handle,
            !"\
                CREATE INDEX \
                    account_id \
                ON \
                    `"DB_LOGS"`(account_id) \
            ", false
        );
        mysql_query(g_log_sql_handle,
            !"\
                ALTER TABLE \
                    `"DB_LOGS"`\
                ADD CONSTRAINT \
                    `log_accounts_fk_1` \
                FOREIGN KEY \
                    (`account_id`) \
                REFERENCES \
                    `"DB_LOGS_ACCOUNTS"` (`"DB_LOGS_ACCOUNT_ID"`) \
                ON DELETE CASCADE ON UPDATE CASCADE \
            ", false
        );
       
        printf("[MySQL]: таблица `"DB_LOGS"` инициализирована за [%d] мс.", 
            GetTickCount() - current_tick
        );
    }
    
    cache_delete(cache_id);
}

stock Log_Add(const playerid, const E_LOG_TYPE: type, const message[], const value = -1)
{
    new 
        query_string[135 + (- 2 + 1) + (- 2 + MAX_PLAYER_NAME) + (- 2 + 11) + (- 2 + MAX_LOG_MESSAGE_LENGHT) + 1],
        player_name[MAX_PLAYER_NAME];

    GetPlayerName(playerid, player_name, MAX_PLAYER_NAME);

    mysql_format(g_log_sql_handle, query_string, sizeof query_string, 
        "\
            INSERT INTO `"DB_LOGS"` \
                (type, name, account_id, log) \
            VALUES \
                (%d, '%e', %d, '%e') \
        ",
            type,
            player_name,
            value,
            message
    );
    mysql_tquery(g_log_sql_handle, query_string, "", "");
    
    #if defined LOG_TELEGRAM_BOT
        Log_SendTelegramBotMessage(type, message);
    #endif

    return true;
}

stock Log_LoadPlayerAccountID(const playerid) 
{
    new query_string[170 + (- 2 + MAX_PLAYER_NAME) + 1];

    mysql_format(g_log_sql_handle, query_string, sizeof query_string, 
        "\
            SELECT \
                "DB_LOGS_ACCOUNT_ID" \
            FROM \
                `"DB_LOGS_ACCOUNTS"` \
            WHERE \
                "DB_LOGS_ACCOUNT_NAME"='%e' \
            LIMIT 1 \
        ",
            g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_NAME]
    );
    mysql_tquery(g_log_sql_handle, query_string, "OnPlayerLoadAccountID", "d", playerid);

    return true;
}

stock Log_ShowPlayerFindDialog(const playerid) 
{
    ShowPlayerDialog(
        playerid, 
        DIALOG_FIND_LOGS, 
        DIALOG_STYLE_INPUT, 
        "{FF9D14}Server Logs", 
        "{FFFFFF}Введите имя аккаунта игрока в диалоговое поле:", 
        "Далее", "Отмена"
    );
    return true;
}

stock Log_LoadPage(const playerid, page)
{
    new query_string[100 + (- 2 + 11) + 1];

    SetPVarInt(playerid, pVar__LogsPageID, page);
    page--;

    format(query_string, sizeof query_string, 
        "\
            SELECT * FROM \
                `"DB_LOGS"` \
            WHERE \
                account_id=%d \
        ",
            g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_ID]
    );
    mysql_tquery(g_log_sql_handle, query_string, "OnPlayerLogsSearch", "dd", playerid, page);

    return true;
}

stock Log_LoadDetails(const playerid, const index)
{
    new query_string[90 + (- 2 + 2) + 1];

    format(query_string, sizeof query_string, 
        "\
            SELECT * FROM \
                `"DB_LOGS"` \
            WHERE \
                id=%d \
        ",
            index
    );
    mysql_tquery(g_log_sql_handle, query_string, "OnPlayerLoadLogsDetails", "d", playerid);

    return true;
}

stock Log_ClearTempData(const playerid)
{
    g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_ID] = NULL_g_player_log_data[E_PLAYER_TEMP_LOG_ID];
    g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_NAME][0] = NULL_g_player_log_data[E_PLAYER_TEMP_LOG_NAME];
    g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_PAGE] = NULL_g_player_log_data[E_PLAYER_TEMP_LOG_PAGE];
}

stock Log_GetTypeName(const E_LOG_TYPE: type, output_string[], const size = sizeof output_string)
{
    if (!(LOG_HOUSE_TYPE <= type <= LOG_ADMIN_TYPE)) {
        return false;
    }

    static const log_type_names[][] = 
    {
        { "Дом" },
        { "Бизнес" },
        { "Фракция" },
        { "Финансы" },
        { "Администратор" }
    };

    strcat(output_string, log_type_names[_:type], size);

    return true;
}

#if defined LOG_TELEGRAM_BOT
stock Log_InitTelegramBot()
{
    g_log_telegram_bot = TG_Connect(LOG_TELEGRAM_BOT_TOKEN);
}

stock Log_SendTelegramBotMessage(const E_LOG_TYPE: type, const message[])
{
	new
        string[MAX_LOG_MESSAGE_LENGHT + 1],
        date_string[20 + 1],
        ret_name[MAX_LOG_TYPE_LENGHT + 1];
    
    new 
        year, month, day,
        hour, minute, second;
    
    getdate(year, month, day);
    gettime(hour, minute, second);

    Log_GetTypeName(type, ret_name);

    format(date_string, sizeof date_string, "%02d:%02d, %02d.%02d.%02d", 
        hour, 
        minute, 
        day, 
        month, 
        year
    );

    format(string, sizeof string, "`Лог | %s`: ***%s***. `%s`", 
        ret_name,
        message,
        date_string
    );
    TG_SendMessage(g_log_telegram_bot, LOG_TELEGRAM_CHAT_ID, string, .parse_mode=MARKDOWN);

    return true;
}
#endif

/*  
    hook OnGameModeInit
*/

public OnGameModeInit()
{
    print("----------------------------------");
    printf("Log system: система успешно загружена. Версия: %.1f", LOG_CURRENT_VERSION);
    print("by: aureliusnull\nСпециально для pawn-wiki.ru");
    print("----------------------------------");

    Log_Init();
    Log_InitTelegramBot();

    #if defined log_OnGameModeInit
        return log_OnGameModeInit();
    #else
        return true;
    #endif
}
#if defined _ALS_OnGameModeInit
    #undef OnGameModeInit
#else
    #define _ALS_OnGameModeInit
#endif

#define OnGameModeInit log_OnGameModeInit
#if defined log_OnGameModeInit
    forward log_OnGameModeInit();
#endif

/*  
	hook OnPlayerConnect
*/

public OnPlayerConnect(playerid)
{
    Log_ClearTempData(playerid);

    #if defined log_OnPlayerConnect
        return log_OnPlayerConnect(playerid);
    #else
        return true;
    #endif
}
#if defined _ALS_OnPlayerConnect
	#undef OnPlayerConnect
#else
	#define _ALS_OnPlayerConnect
#endif

#define OnPlayerConnect log_OnPlayerConnect
#if	defined log_OnPlayerConnect
	forward log_OnPlayerConnect(playerid);
#endif

/* 
    hook OnDialogResponse
*/

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    switch (dialogid) {
        case DIALOG_FIND_LOGS: {
            if (!response) {
                return true;
            }
            
            new length = strlen(inputtext);

            if (!length || length > MAX_PLAYER_NAME) {
                Log_ShowPlayerFindDialog(playerid);
                return true;
            }

            strcat(g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_NAME], inputtext, MAX_PLAYER_NAME);

            Log_LoadPlayerAccountID(playerid);
            return true;
        }
        case DIALOG_LOAD_LOGS_PAGE: {
            new page = GetPVarInt(playerid, pVar__LogsPageID);

            if (!response) {
                if (page <= 1) {
                    Log_ShowPlayerFindDialog(playerid);
                } else {
                    Log_LoadPage(playerid, page - 1);
                }
                return true;
            }
            else if (!listitem) {
                Log_LoadPage(playerid, page + 1);
                return true;
            }

            new index = g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_PAGE][listitem - 1];
            
            Log_LoadDetails(playerid, index);
            return true;
        }
        case DIALOG_LOAD_LOGS_DETAIL: {
            if (response) {
                DeletePVar(playerid, pVar__LogsPageID);
                Log_ClearTempData(playerid);
                return true;
            }

            return true;
        }
    }

    #if defined log_OnDialogResponse
		return log_OnDialogResponse(playerid, dialogid, response, listitem, inputtext);
	#else
		return true;
	#endif
}
#if defined _ALS_OnDialogResponse
	#undef OnDialogResponse
#else
	#define _ALS_OnDialogResponse
#endif

#define OnDialogResponse log_OnDialogResponse
#if defined log_OnDialogResponse
	forward log_OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]);
#endif

/* 
    callbacks
*/

public OnPlayerLogsSearch(playerid, page)
{
    new 
        row_count = cache_num_rows(),
        dialog_string[(94 + (- 2 + 2) + (- 2 + 20)) * MAX_LOGS_ON_PAGE + 1] = "{FFFFFF}ID:\t{FFFFFF}Дата создания\n",
        tmp_id,
        tmp_date[20 + 1],
        count;

    if (row_count > page * MAX_LOGS_ON_PAGE) {
 		format(dialog_string, sizeof dialog_string, "%s{FF6347}>> Страница %d{FFFFFF}\n", 
            dialog_string, 
            page + 1
        );
    }

	if (row_count <= page * MAX_LOGS_ON_PAGE) {
		return SendClientMessage(playerid, -1, (!row_count) ? ("На указнный ник игрока отсутствуют логи.") : ("Больше нет страниц с логами аккаунта."));
    }

    for (new i = page * MAX_LOGS_ON_PAGE; i < row_count; ++i) {
        if (++count == MAX_LOGS_ON_PAGE + 1) {
            break;
        }

        cache_get_value_name_int(i, "id", tmp_id);
        cache_get_value_name(i, "date", tmp_date, 20);

        g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_PAGE][count - 1] = tmp_id;

        format(dialog_string, sizeof dialog_string, "%s{00BFFF}№-%d\t{FFFF00}%s\n", 
            dialog_string,
            tmp_id,
            tmp_date
        );
    }

    ShowPlayerDialog(
        playerid, 
        DIALOG_LOAD_LOGS_PAGE, 
        DIALOG_STYLE_TABLIST_HEADERS, 
        "{FF9D14}Server Logs",
        dialog_string, 
        "Выбрать", "Назад"
    );
    return true;
}

public OnPlayerLoadLogsDetails(playerid)
{
    new row_count = cache_num_rows();

    if (!row_count) {
        return true;
    }

    new 
        dialog_string[150 + (- 2 + MAX_PLAYER_NAME) + (- 2 + 11) + (- 2 + MAX_LOG_TYPE_LENGHT) + (- 2 + MAX_LOG_MESSAGE_LENGHT) + 1],
        header_string[24 + (- 2 + 3) + 1],
        ret_name[MAX_LOG_TYPE_LENGHT + 1],
        tmp_id,
        tmp_type,
        tmp_account_id,
        tmp_name[MAX_PLAYER_NAME + 1],
        tmp_content[MAX_LOG_MESSAGE_LENGHT + 1];

    cache_get_value_name_int(0, "id", tmp_id);
    cache_get_value_name_int(0, "type", tmp_type);
    cache_get_value_name_int(0, "account_id", tmp_account_id);

    cache_get_value_name(0, "name", tmp_name);
    cache_get_value_name(0, "log", tmp_content);

    Log_GetTypeName(E_LOG_TYPE:tmp_type, ret_name);

    format(header_string, sizeof header_string, "{FF9D14}Лог: {FFFFFF}#%d", 
        tmp_id
    );

    format(dialog_string, sizeof dialog_string, 
    "\
        {FFFFFF}Ник: {FF9D14}%s{FFFFFF}\n\
        ID аккаунта: {FF9D14}%d{FFFFFF}\n\
        Тип действия: {FF9D14}%s{FFFFFF}\n\
        Контент: {FF9D14}%s\
    ", 
        tmp_name,
        tmp_account_id,
        ret_name,
        tmp_content
    );

    ShowPlayerDialog(
        playerid, 
        DIALOG_LOAD_LOGS_DETAIL, 
        DIALOG_STYLE_MSGBOX, 
        header_string,
        dialog_string, 
        "Закрыть", ""
    );
    return true;
}

public OnPlayerLoadAccountID(playerid)
{
    new row_count = cache_num_rows();

    if (!row_count) {
        return true;
    }

    cache_get_value_name_int(0, ""DB_LOGS_ACCOUNT_ID"", g_player_temp_log_data[playerid][E_PLAYER_TEMP_LOG_ID]);

    Log_LoadPage(playerid, 1);
    return true;
}