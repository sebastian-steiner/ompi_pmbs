#!/usr/bin/perl -w

#
# Copyright (c) 2013-2015 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2013-2015 Inria.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

#
# Author Emmanuel Jeannot <emmanuel.jeannot@inria.fr>
#
# This script aggregates the profiles generated by the flush_monitoring function.
# The files need to be in in given format: name_<phase_id>_<process_id>
# They are then aggregated by phases.
# If one needs the profile of all the phases he can concatenate the different files,
# or use the output of the monitoring system done at MPI_Finalize
# in the example it should be call as:
# ./aggregate_profile.pl prof/phase to generate
# prof/phase_1.prof
# prof/phase_2.prof
#
# ensure that this script as the executable right: chmod +x ...
#

die "$0 <name of the  profile>\n\tProfile files should be of the form  \"name_phaseid_processesid.prof\"\n\tFor instance if you saved the monitoring into phase_0.0.prof, phase_0.1.prof, ..., phase_1.0.prof etc you should call: $0 phase\n" if ($#ARGV!=0);

$name = $ARGV[0];

@files = glob ($name."*");

%phaseid = ();


# Detect the different phases
foreach $file (@files) {
  ($id)=($file =~ m/$name\_(\d+)\.\d+/);
  $phaseid{$id} = 1 if ($id);
}

# for each phases aggregate the files
foreach $id (sort {$a <=> $b} keys %phaseid) {
  aggregate($name."_".$id);
}




sub aggregate{
  $phase = $_[0];

  print "Building $phase.prof\n";

  open OUT,">$phase.prof";

  @files = glob ($phase."*");

  foreach $file ( @files) {
    open IN,$file;
    while (<IN>) {
      print OUT;
    }
    close IN;
  }
  close OUT;
}
