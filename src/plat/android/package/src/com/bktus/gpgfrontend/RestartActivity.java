// SPDX-License-Identifier: GPL-3.0-or-later

package com.bktus.gpgfrontend;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Process;

/**
 * Starts GpgFrontend again in a new process (PlatformBridge.restart()).
 *
 * It runs in a process of its own (":restart" in the manifest), so it can
 * end the application's, which has already shut its core down, and then open
 * the launcher's activity: that comes up in a fresh process, which chooses
 * its profile as it starts. Started while the application is in front, so it
 * may start an activity itself. Shows nothing.
 */
public class RestartActivity extends Activity {
    static final String EXTRA_PID = "com.bktus.gpgfrontend.restart.PID";
    static final String EXTRA_LAUNCH = "com.bktus.gpgfrontend.restart.LAUNCH";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        int pid = getIntent().getIntExtra(EXTRA_PID, -1);
        if (pid > 0 && pid != Process.myPid()) Process.killProcess(pid);

        Intent launch = getIntent().getParcelableExtra(EXTRA_LAUNCH);
        if (launch != null) {
            launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_CLEAR_TASK);
            startActivity(launch);
        }

        finish();
        // Nothing of this process is needed any more.
        Runtime.getRuntime().exit(0);
    }
}
