#!/usr/bin/perl -w

# reads PID on stdin
# writes short command description on stdout (command name by default)
# to be used as atop pidtocmd helper
# 2017

$| = 1;
IN: while (<>) {
    chomp;
    my $pid=$_;

    my $default_name;
    my $pstat_path="/proc/".$pid."/stat";
    if (open (my $pstat, "<", $pstat_path)) {
        my $pstat_line=<$pstat>;
        close ($pstat);
        $default_name = ($pstat_line =~ /^\d+\s*\((.*?)\)/) ? $1 : "?";
    } else {
        $default_name = "?";
    }
    # from here, $default_name is always defined

    my $pcmd_line;
    my $pcmd_path="/proc/".$pid."/cmdline";
    if (open (my $pcmd, "<", $pcmd_path)) {
        $pcmd_line=<$pcmd>;
        close ($pcmd);
    }

####  specific processes

    # vmware: VM name
    if ((defined $pcmd_line) && ($pcmd_line =~ /.*(^|\/)vmware\-vmx.*\/(.*?)\.vmx\x00/)) {
        print $2."\n";
        next IN;
    }

    # java: first non-option
    if ((defined $pcmd_line) && ($pcmd_line =~ /.*(^|\/)java\x00/)) {
        for (split ("\x00", $pcmd_line)) {
            if ($_ !~ /(^\-)|((^|\/)java)/) {
                print $_."\n";
                next IN;
            }
        }
    }

    # loop device: upper-level dm device
    if ($default_name =~ /^loop\d$/) {
        my $dmdev = `cat /sys/block/$default_name/holders/*/dm/name 2>/dev/null`;
        if ($dmdev) {
            print $dmdev;
            next IN;
        }
    }

####

    print $default_name."\n";
}

