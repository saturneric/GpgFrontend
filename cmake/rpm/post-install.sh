#!/bin/sh

# Update icon cache
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    /usr/bin/gtk-update-icon-cache -f -t /usr/share/icons/hicolor || :
fi

# Update desktop database
if [ -x /usr/bin/update-desktop-database ]; then
    /usr/bin/update-desktop-database -q /usr/share/applications || :
fi

# Update mime database
if [ -x /usr/bin/update-mime-database ]; then
    /usr/bin/update-mime-database /usr/share/mime || :
fi

# Try XDG icon cache update if available
if [ -x /usr/bin/xdg-icon-resource ]; then
    /usr/bin/xdg-icon-resource forceupdate --theme hicolor || :
fi

# Clear icon cache for current user if logged in
if [ ! -z "$SUDO_USER" ]; then
    su "$SUDO_USER" -c 'gtk-update-icon-cache -f -t ~/.local/share/icons/hicolor 2>/dev/null || :'
fi 