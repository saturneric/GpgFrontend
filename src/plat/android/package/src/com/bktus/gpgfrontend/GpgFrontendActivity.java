// SPDX-License-Identifier: GPL-3.0-or-later

package com.bktus.gpgfrontend;

import android.content.Intent;
import android.os.Bundle;

import org.qtproject.qt.android.bindings.QtActivity;

/**
 * Qt's activity, which also keeps what other applications share with
 * GpgFrontend (PlatformBridge.receive()) until the application takes it.
 * One instance in a task of its own (launchMode singleTask): a share from
 * another application brings it forward with onNewIntent() instead of
 * starting a second Qt activity in the sender's task.
 */
public class GpgFrontendActivity extends QtActivity {
    /**
     * The extra Qt's loader reads the application's arguments from, in any
     * build. The activity is exported (it is the launcher's, and takes
     * shares), so any application could start it with arguments of its
     * choosing: --temp-profile and the like choose the profile. GpgFrontend
     * is never started with arguments on a phone.
     */
    private static final String EXTRA_QT_ARGUMENTS = "applicationArguments";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // Before Qt's loader reads it.
        Intent started = getIntent();
        if (started != null) started.removeExtra(EXTRA_QT_ARGUMENTS);
        super.onCreate(savedInstanceState);
        PlatformBridge.releasePersistedGrants(this);
        // The loading page, until the application draws it (gf_splash): Qt
        // may set a theme of its own, whose window is plain white.
        int splash = getResources().getIdentifier(
                "gf_splash", "drawable", getPackageName());
        if (splash != 0) getWindow().setBackgroundDrawableResource(splash);
        // A share is taken once: not again when the activity is recreated
        // (rotated, brought back after its process was ended) or reopened
        // from Recents, which repeat the intent it was started with.
        boolean fromHistory = (getIntent().getFlags()
                & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) != 0;
        if (savedInstanceState == null && !fromHistory) take(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        take(intent);
        // A page already on screen takes it now, not on the next resume.
        PlatformBridge.announceShare();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // GpgFrontend's own picker; anything else is Qt's.
        if (requestCode == PlatformBridge.REQUEST_PICK_DOCUMENTS) {
            PlatformBridge.documentsPicked(resultCode, data);
            return;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void take(Intent intent) {
        PlatformBridge.receive(this, intent);
        // Taken: what the activity is kept with no longer names it.
        setIntent(new Intent(Intent.ACTION_MAIN));
    }
}
