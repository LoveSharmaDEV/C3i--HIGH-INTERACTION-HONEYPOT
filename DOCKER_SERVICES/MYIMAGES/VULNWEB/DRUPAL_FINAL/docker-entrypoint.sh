#!/bin/bash

set -e

if [[ "$1" == apache2* ]] || [ "$1" == php-fpm ]; then
        if [ -n "$MYSQL_PORT_3306_TCP" ]; then
                if [ -z "$DRUPAL_DB_HOST" ]; then
                        DRUPAL_DB_HOST='mysql'
                else
                        echo >&2 "warning: both DRUPAL_DB_HOST and MYSQL_PORT_3306_TCP found"
                        echo >&2 "  Connecting to DRUPAL_DB_HOST ($DRUPAL_DB_HOST)"
                        echo >&2 "  instead of the linked mysql container"
                fi
        fi

        if [ -z "$DRUPAL_DB_HOST" ]; then
                echo >&2 "error: missing DRUPAL_DB_HOST and MYSQL_PORT_3306_TCP environment variables"
                echo >&2 "  Did you forget to --link some_mysql_container:mysql or set an external db"
                echo >&2 "  with -e DRUPAL_DB_HOST=hostname:port?"
                exit 1
        fi

        # If the DB user is 'root' then use the MySQL root password env var
        : ${DRUPAL_DB_USER:=root}
        if [ "$DRUPAL_DB_USER" = 'root' ]; then
                : ${DRUPAL_DB_PASSWORD:=$MYSQL_ENV_MYSQL_ROOT_PASSWORD}
        fi
        : ${DRUPAL_DB_NAME:=joomla}

        if [ -z "$DRUPAL_DB_PASSWORD" ] && [ "$DRUPAL_DB_PASSWORD_ALLOW_EMPTY" != 'yes' ]; then
                echo >&2 "error: missing required DRUPAL_DB_PASSWORD environment variable"
                echo >&2 "  Did you forget to -e DRUPAL_DB_PASSWORD=... ?"
                echo >&2
                echo >&2 "  (Also of interest might be DRUPAL_DB_USER and DRUPAL_DB_NAME.)"
                exit 1
        fi

        if ! [ -e index.php -a \( -e libraries/cms/version/version.php -o -e libraries/src/Version.php \) ]; then
                echo >&2 "drupal not found in $(pwd) - copying now..."

                if [ "$(ls -A)" ]; then
                        echo >&2 "WARNING: $(pwd) is not empty - press Ctrl+C now if this is an error!"
                        ( set -x; ls -A; sleep 10 )
                fi

                tar cf - --one-file-system -C /usr/src/drupal . | tar xf -

                if [ ! -e .htaccess ]; then
                        # NOTE: The "Indexes" option is disabled in the php:apache base image so remove it as we enable .htaccess
                        sed -r 's/^(Options -Indexes.*)$/#\1/' htaccess.txt > .htaccess
                        chown www-data:www-data .htaccess
                fi

                echo >&2 "Complete! drupal has been successfully copied to $(pwd)"
        fi

        # Ensure the MySQL Database is created
        php /makedb.php "$DRUPAL_DB_HOST" "$DRUPAL_DB_USER" "$DRUPAL_DB_PASSWORD" "$DRUPAL_DB_NAME"

        echo >&2 "========================================================================"
        echo >&2
        echo >&2 "This server is now configured to run drupal!"
        echo >&2
        echo >&2 "NOTE: You will need your database server address, database name,"
        echo >&2 "and database user credentials to install Joomla."
        echo >&2
        echo >&2 "========================================================================"
fi

exec "$@"
