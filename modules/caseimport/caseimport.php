<?php
include 'preg_find.php';
//include 'filecheckdirectory.php';
//include 'filecopy.php';
include 'file.inc';
include 'bootstrap.inc';

function get_string_between($string, $start, $end){
        $string = " ".$string;
        $ini = strpos($string,$start);
        if ($ini == 0) return "";
        $ini += strlen($start);
        $len = strpos($string,$end,$ini) - $ini;
        return substr($string,$ini,$len);
}

$files = preg_find('/(.html)$/', '/home/kerry/Seychelles/sc', PREG_FIND_RECURSIVE);
foreach($files as $file) {
        echo "$file\n" ;
        list($fname, $ext) = explode(".", $file);
        $pdf_file = $fname . ".pdf";
        $rtf_file = $fname . ".rtf";
        $base_pdf_file = basename($pdf_file);
        $base_rtf_file = basename($rtf_file);
        echo "pdf_file: $pdf_file rtf_file: $rtf_file\n";
echo "base_pdf_file: $base_pdf_file base_rtf_file: $base_rtf_file\n";
        list($jno, $fext) = explode(".", $base_pdf_file);
        system("/usr/bin/fromdos -dov $file");
        $fh = fopen($file, "r") or die("Can't open file");
        $fullstring = fread($fh, filesize($file));
        //$fullstring = "this is my <title>dog</title>";
        $parsed = get_string_between($fullstring, "<title>", "</title>");
        $title = htmlspecialchars_decode(get_string_between($fullstring, "<title>", "["));
        preg_match("/[0-9]* of [0-9]*/", $parsed, $matches);
        $caseno = $matches[0];
//      $caseno = get_string_between($parsed, "(", ")");
        preg_match("/\[[0-9]{4}\] SC[A-Z]{2} [0-9]*/", $parsed, $matches2) ;
        $mnc = $matches2[0];
 $courtname = "Supreme Court";
        $cname = "supreme-court";
        echo "courtname: $courtname\n";
        $pre_pre_judgment_date = get_string_between($fullstring, "(", "<meta>");
        $pre_judgment_date = get_string_between($pre_pre_judgment_date, "]", "</title>");
        $judgment_date = get_string_between($pre_judgment_date, "(", ")");
        //$judgment_date = date("j F, Y"); 
        $year = get_string_between($parsed, "[", "]");
        preg_match("/<!--sino date [0-9]* [A-Z][a-z]* [0-9]{4}-->/", $fullstring, $sinomatches);
$sinomatch = $sinomatches[0];
        echo "sinomatch $sinomatch\n";
        $body = get_string_between($fullstring, $sinomatch, "<!--sino noindex-->");
        //echo "parsed: $parsed\n";
        echo "title: $title\n";
        echo "caseno: $caseno\n";
        echo "mnc: $mnc\n";
        echo "judgment_date: $judgment_date\n";
        echo "jno: $jno\n";
        echo "year: $year\n";
//      echo "body: $body\n";

	$unixdate = strtotime($judgment_date);
        $udate = date("Y-m-d H:i:s", $unixdate);
        echo "unixdate: $unixdate\n";

// now insert into database
	$node = new stdClass();	
	$node->uid = 1;
	$node->name = $title;
	$node->title = $title;
	$node->body = $body;
	$node->type = 'caselaw';
	$node->created = time();
	$node->changed = $node->created;
	$node->promote = 0;
	$node->sticky = 0;
	$node->format = 2;
	$node->status = 1;
	$node->language = 'en';
	$node->field_courtname[0]['value'] = $courtname;
	$node->field_mnc[0]['value'] = $mnc;
	$node->field_jdate[0]['value'] = $unixdate;
	$node->field_judge[0]['value'] = '';
	$node->field_jno[0]['value'] = $jno;
	$node->field_caseno[0]['value'] = $caseno;

	if(file_exists($pdf_file)) {
        $pdf_filesize = filesize($pdf_file);
	$dest = "sites/default/files";
        $target = "sc/judgment/" . $cname . "/" . $year . "/" . $jno . "/";
	// if new directory path does not exist, create it
	$new_dir_path = $dest . "/" . $target;
	file_check_directory($new_dir_path, $mode = FILE_CREATE_DIRECTORY);
  
	
 	// Copy the file to the Drupal files directory 
    	if(!file_copy($pdf_file,$target)) {
        	echo "Failed to move file: $pdf_file.\n";
        //	return;
    	} else {
		echo "copied file: $pdf_file.\n";
        // file_move might change the name of the file
        	$fname = basename($pdf_file);
    	}

	$pdffile = new stdClass();
	$pdffile->filename = $fname;
	$pdffile->filepath = $target .$fname;
	$pdffile->filemime = file_get_mimetype($fname);
	$pdffile->filesize = $pdf_filesize;
	$pdffile->filesource = $fname;
	$pdffile->uid = 1;
	$pdffile->status = FILE_STATUS_TEMPORARY;
	$pdffile->timestamp = time();
	$pdffile->list = 1;
	$pdffile->description = $fname;
	$pdffile->new = true;
	
	drupal_write_record('files', $pdffile);
	file_set_status($pdffile,1);
	$node->files[$pdffile->fid] = $pdffile;
        }

	if(file_exists($rtf_file)) {
	$rtf_filesize = $details['size'];
	$dest = "sites/default/files";
        $target = "sc/judgment/" . $cname . "/" . $year . "/" . $jno . "/";
	// if new directory path does not exist, create it
	$new_dir_path = $dest . "/" . $target;
	file_check_directory($new_dir_path, $mode = FILE_CREATE_DIRECTORY);

	//copy the file to the Drupal files directory
	if(!file_copy($rtf_file,$target)) {
		echo "Failed to move file: $rtf_file.\n";
	} else {
		echo "copied rtf file: $rtf_file.\n";
	// file move might change the name of the file
		$fname2 = basename($rtf_file);
	}		

	$rtffile = new stdClass();
	$rtffile->filename = $fname2;
	$rtffile->filepath = $target . $fname2;
	$rtffile->filemime = file_get_mimetype($fname2);
	$rtffile->filesize = $rtf_filesize;
	$rtffile->filesource = $fname2;
	$rtffile->uid = 1;
	$rtffile->status = FILE_STATUS_TEMPORARY;
	$rtffile->timestamp = time();
	$rtffile->list = 1;
	$rtffile->description = $fname2;
	$rtffile->new = true;

	drupal_write_record('files', $rtffile);
	file_set_status($rtffile, 1);
	$node->files[$rtffile->fid] = $rtffile;
	}

	if ($node = node_submit($node)) {
	    node_save($node);
	    drupal_set_message(t("Node ".$node->title." added correctly"));
	} else {
	    drupal_set_message(t("Node ".$node->title." added incorrectly"), "error");
	}
//	return $node;
//	return true;
}
