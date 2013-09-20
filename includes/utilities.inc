<?php

/**
 * Send documents to Calliope to create MVD.
 * @param String $critical_edition
 *   identifier for MVD
 * @param array $resource_ids
 *   array of transcription identifiers
 *
 * @return String
 *   Success/failure message
 */
function islandora_calliope_create_mvd($critical_edition, $transcriptions = array()) {
  // temp, until we can get calliope running on this box
  $doctitle = "english/alan/" . str_replace(':', '_', $critical_edition);
  $base_url = "http://192.168.168.63";
  $base_url = "http://experimental.emic.dgicloud.com";
  $calliope_import = "$base_url/import/";
  $boundary = "-------------" . uniqid();
  $data = "";
  $id_json_array = "";
  foreach ($transcriptions as $transcription) {
    if ($transcription != "") {
      if ($id_json_array) {
        $id_json_array.=",";
      }
      $object = islandora_object_load($transcription);
      $versionid = str_replace(':', '_', $object->label);
      $id_json_array.= "{\"id\":\"$transcription\",\"name\":\"$versionid\"}";      
      // load contents for resource
  
      $contents = $object['TRANSCRIPTION']->content;
      $contenttype = 'text/plain';
      $data .= "--" . $boundary . "\r\n";
      $data .= "Content-Disposition: form-data; name=\"uploadedfile[]\"; filename=\"" . $versionid . "\"\r\n";
      $data .= "Content-Type: " . $contenttype . "\r\n";
      $data .= "Content-Length: " . strlen($contents) . "\r\n";
      $data .= "Content-Transfer-Encoding: binary\r\n\r\n";
      $data .= $contents . "\r\n";
    }
  }
  $data .= "--" . $boundary . "\r\n";
  $data .= "Content-Disposition: form-data; name=\"DOC_ID\"\r\n\r\n" . $doctitle . "\r\n";
  $data .= "--" . $boundary . "--\n";

  // post to calliope server import
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, trim($calliope_import));
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
  curl_setopt($ch, CURLOPT_POST, 1);
  // Add data to curl request.
  curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
  curl_setopt($ch, CURLOPT_HTTPHEADER, array("Content-type: multipart/form-data; boundary=" . $boundary));
  // get back status or response
  $response = curl_exec($ch);
  $err = curl_errno($ch);
  $error = curl_error($ch);
  curl_close($ch);

  if ($err) {
    return t('Error: ' . $err . " " . $error . " " . $response);
  }
  else {
    $message = "";

// all docdata has been retrieved at this point    
    $hriterror = preg_match('/rror/', $response);
    if ($hriterror == 0) {
      $hriterror = preg_match('/HritServer Error/', $response);
      if ($hriterror == 0) {
        // store MVD details in db for now
        // parse existing mvd results to check if one exists with same name, get id and delete
        $mvd_search_url = $base_url . "/" . drupal_get_path('module', 'repository') . "/api/mvds/?searchField=name&q=" . $doctitle . "$";
        $mvd_search = file_get_contents($mvd_search_url);
        $mvd_search_json = json_decode($mvd_search, TRUE);
        $existing_mvd_count = $mvd_search_json['count'];
        if ($existing_mvd_count != 0) {
          $existing_mvd = $mvd_search_json['results'][0]['id'];
          $ch3 = curl_init();
          curl_setopt($ch3, CURLOPT_CUSTOMREQUEST, 'DELETE');
          curl_setopt($ch3, CURLOPT_URL, trim($base_url . "/" . drupal_get_path('module', 'repository') . "/api/mvds/" . $existing_mvd));
          $db_response = curl_exec($ch3);
          $dberr = curl_errno($ch3);
          $dberror = curl_error($ch3);
          curl_close($ch3);
          if ($dberr) {
            return t('Error: ' . $dberr . " " . $dberror . $db_response);
          }
        }
        // post new MVD details to db
        $mvd_db_url = $base_url . "/" . drupal_get_path('module', 'repository') . "/api/mvds/";
        $mvd_json = "{\"name\":\"$critical_edition\",\"resources\":[$id_json_array]}";
        $ch2 = curl_init();
        curl_setopt($ch2, CURLOPT_URL, trim($mvd_db_url));
        curl_setopt($ch2, CURLOPT_RETURNTRANSFER, 1);
        curl_setopt($ch2, CURLOPT_FOLLOWLOCATION, 1);
        curl_setopt($ch2, CURLOPT_POST, 1);
        curl_setopt($ch2, CURLOPT_POSTFIELDS, $mvd_json);
        curl_setopt($ch2, CURLOPT_HTTPHEADER, array("Content-type: application/json;"));

        $db_response = curl_exec($ch2);
        $dberr = curl_errno($ch2);
        $dberror = curl_error($ch2);
        curl_close($ch2);
        if ($dberr) {
          return t('Error: ' . $dberr . " " . $dberror . $db_response);
        }

        // message is success message
        $message = "<p>MVD created. You may now view <a href='/collationtools/apparatus#$critical_edition'>table apparatus</a> or <a href='/collationtools/compare#$critical_edition'>side-by-side comparison</a></p>";
      }
      else {
        $message = "<p>MVD was not created. Refer to the log above for the cause.";
      }
    }
    else {
      // else message is failure message
      $message = "<p>There was an error creating the MVD. Refer to the log above for the cause. If the log does not indicate a reason, the operation may have failed as a result of versions being too large or too much variation between versions. Try splitting larger transcriptions into smaller sections.</p>";
    }
    return t('<p>' . $response . '</p>' . $message);
  }
}