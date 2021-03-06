<?php
// $Id: custom_breadcrumbs_taxonomy.inc,v 1.1.2.19 2010/12/30 18:36:25 mgn Exp $

/**
 * @file
 * Helper functions for custom_breadcrumbs_taxonomy.
 */

/**
 * Sets a taxonomy breadcrumb and calls the original taxonomy/term/% callback.
 *
 * @param $callback
 *   A callback function as defined in hook_menu().
 * @param $file
 *   A callback file as defined in hook_menu().
 * @param $filepath
 *   A callback file path as defined in hook_menu().
 * @param $str_tids
 *   A term selector string, e.g. "1,3,8" or "4+9".
 * @param ...
 *   Additional arguments to pass on to the callback.
 *
 * @return
 *   The return value of the original callback function.
 */
function custom_breadcrumbs_taxonomy_term_page($str_tids, $callback, $file, $filepath) {
  $args = array_slice(func_get_args(), 4);

  // Use first term to generate breadcrumb trail.
  $tids = taxonomy_terms_parse_string($str_tids);
  $terms = array();
  foreach ($tids['tids'] as $tid) {
    $term = taxonomy_get_term($tid);
    if ($term) {
      $terms[$term->tid] = $term;
    }
  }
  if (is_file($filepath . '/' . $file)) {
    require_once($filepath . '/' . $file);
  }
  $output = call_user_func_array($callback, $args);
  _custom_breadcrumbs_taxonomy_set_breadcrumb($tids['tids'][0], NULL, TRUE, array(), $terms);

  return $output;
}

/**
 * Sets the breadcrumb using a node's taxonomy.
 *
 * @param $tid
 *   A taxonomy id.
 * @param $vid
 *   A taxonomy vocabulary id.
 * @param $is_term_page
 *   TRUE if the breadcrumb is being prepared for the taxonomy term page, FALSE otherwise.
 * @param $objs
 *   An optional array of objects to be used for token replacement.
 * @param $terms
 *   An array of taxonomy terms to use (potentially) to construct the breadcrumb.
 */
function _custom_breadcrumbs_taxonomy_set_breadcrumb($tid, $vid = NULL, $is_term_page = FALSE, $objs = array(), $terms = array()) {
  if (variable_get('custom_breadcrumbs_taxonomy_use_hierarchy', TRUE) && (!custom_breadcrumbs_exclude_path())) {
    $breadcrumb = custom_breadcrumbs_taxonomy_generate_breadcrumb($tid, $vid, $is_term_page, $objs);
    if ($is_term_page) {
      _custom_breadcrumbs_taxonomy_recent_term($tid);
    }
    if (variable_get('custom_breadcrumbs_taxonomy_include_node_title', FALSE) && isset($objs['node'])) {
      $breadcrumb[] = check_plain($objs['node']->title);
    }
    drupal_set_breadcrumb($breadcrumb);
    // Optionally save the unique breadcrumb id of the last set breadcrumb.
    custom_breadcrumbs_unique_breadcrumb_id('taxonomy');
  }
  else {
    global $language;
    $languages = array('language' => $language->language, 'all' => '');
    // Check each term to see if it has a custom breadcrumb.
    $vids = array();
    if (!empty($terms)) {
      foreach ($terms as $term) {
        $breadcrumbs = custom_breadcrumbs_load_breadcrumbs('custom_breadcrumbs_taxonomy', 'custom_breadcrumbs_taxonomy_term', array('tid' => $term->tid), $languages);
        $objs['taxonomy'] = $term;
        if ($breadcrumb = custom_breadcrumbs_select_breadcrumb($breadcrumbs, $objs)) {
          custom_breadcrumbs_set_breadcrumb($breadcrumb, $objs);
          _custom_breadcrumbs_taxonomy_recent_term($term->tid);
          return;
        }
        if (!isset($vids[$term->vid])) {
          $vids[$term->vid] = $term;
        }
      }
    }
    if (empty($vids) && !is_null($vid)) {
      $vids[$vid] = NULL;
    }
    // No terms match, look for a match on the taxonomy vocabulary.
    foreach ($vids as $vid => $term) {
      $breadcrumbs = custom_breadcrumbs_load_breadcrumbs('custom_breadcrumbs_taxonomy', 'custom_breadcrumbs_taxonomy_vocabulary', array('vid' => $vid), $languages);
      $objs['taxonomy'] = $term;
      if ($breadcrumb = custom_breadcrumbs_select_breadcrumb($breadcrumbs, $objs)) {
        custom_breadcrumbs_set_breadcrumb($breadcrumb, $objs);
        return;
      }
    }
  }
}

/**
 * Returns the previous selected term or the lightest term for a given node.
 *
 * @param $node
 *   The node object.
 *
 * @return
 *   The taxonomy term object.
 */
function custom_breadcrumbs_taxonomy_node_get_term($node) {
  // First try to see if a recently viewed term matches one of the node's terms.
  $term = custom_breadcrumbs_taxonomy_node_get_recent_term($node);
  // If not, then select the nodes lightest term.
  if (is_null($term)) {
    $term = custom_breadcrumbs_taxonomy_node_get_lightest_term($node);
  }
  return $term;
}

/**
 * Returns the most recently selected term for a given node.
 *
 * @param $node
 *   The node object.
 *
 * @return
 *   The previously selected taxonomy term object that also belongs to the node.
 */
function custom_breadcrumbs_taxonomy_node_get_recent_term($node) {
  $terms = taxonomy_node_get_terms($node);
  $tid = _custom_breadcrumbs_taxonomy_recent_term();
  if (is_array($terms) && !empty($terms) && !is_null($tid)) {
    return (isset($terms[$tid])) ? taxonomy_get_term($tid) : NULL;
  }
}

/**
 * Sets or returns the previous selected term id.
 *
 * @param $tid
 *   An optional term id to store in the session variable to establish a term history.
 *
 * @return
 *   If this function is called without a term id, then it will return the previously
 *   selected taxonomy term id, retrieved from the session variable.
 */
function _custom_breadcrumbs_taxonomy_recent_term($tid = NULL) {
  if (!is_null($tid)) {
    $_SESSION['custom_breadcrumbs_previous_term'] = $tid;
  }
  elseif (isset($_SESSION['custom_breadcrumbs_previous_term'])) {
    return $_SESSION['custom_breadcrumbs_previous_term'];
  }
}

/**
 * Returns the lightest term for a given node.
 *
 * If the term has parents, then the lightest parent's weight is used for the
 * term weight. And if the parent has multiple child terms at different depths,
 * the deepest child term will be returned. If the child terms have the same
 * depth, then the lightest child term is returned.
 *
 * @param $node
 *   The node object.
 *
 * @return
 *   The taxonomy term object.
 */
function custom_breadcrumbs_taxonomy_node_get_lightest_term($node) {
  $terms = taxonomy_node_get_terms($node);
  if (!empty($terms)) {
    if (count($terms) > 1) {
      foreach ($terms as $term) {
        // Only consider terms in the lightest vocabulary. 
        if (!isset($vid)) {
          $vid = $term->vid;
        }
        elseif ($term->vid != $vid) continue;
        // If the term has parents, the weight of the term is the weight of the lightest parent.
        $parents = taxonomy_get_parents_all($term->tid);
        $depth = count($parents);
        if ($depth > 0) {
          $parent = array_pop($parents);
          $weight = $parent->weight;
        }
        else {
          $weight = $term->weight;
        }
        if ((isset($lweight) && ($weight < $lweight)) || !isset($lweight)) {
          $lterm = $term;
          $lweight = $weight;
          $ldepth = $depth;
        }
        elseif (isset($lweight) && ($weight == $lweight)) {
          // If the node has multiple child terms with the same parent, choose the child with the greatest depth.
          if ($depth > $ldepth) {
            $lterm = $term;
            $ldepth = $depth;
          }
          elseif ($depth == $ldepth) {
            // If the terms have the same depth, pick the term with the lightest weight.
            $lterm = ($lterm->weight < $term->weight) ? $lterm : $term;
          }
        }
      }
      return $lterm;
    }
    else {
      return array_pop($terms);
    }
  }
}

/**
 * Generates a breadcrumb from the taxonomy hierarchy of the term id or vocab id.
 * This will only be called if custom_breadcrumbs_taxonomy_use_hierarchy has been enabled.
 *
 * @param $tid
 *   A taxonomy id.
 * @param $vid
 *   A taxonomy vocabulary id.
 * @param $is_term_page
 *   TRUE if the breadcrumb is being prepared for the taxonomy term page, FALSE otherwise.
 * @param $objs
 *   An optional array of objects to be used for token replacement.
 *
 * @return
 *   The breadcrumb trail.
 */
function custom_breadcrumbs_taxonomy_generate_breadcrumb($tid, $vid = NULL, $is_term_page = FALSE, $objs = NULL) {
  $term = (is_null($tid)) ? NULL : taxonomy_get_term($tid);
  $vocabid = (!is_null($vid)) ? $vid : (is_null($term) ? NULL : $term->vid);
  $types = NULL;
  if (module_exists('token')) {
    $objs['taxonomy'] = $term;
    $types = custom_breadcrumbs_token_types($objs);
  }
  $trail = array();
  $trail = _custom_breadcrumbs_taxonomy_home_trail();

  if (!is_null($vocabid)) {
    $vocab_trail = _custom_breadcrumbs_taxonomy_vocabulary_trail($vocabid, $is_term_page, $objs, $types, count($trail));
    $trail = array_merge($trail, $vocab_trail);
  }

  if (!is_null($tid)) {
    $term_trail = _custom_breadcrumbs_taxonomy_term_trail($tid, $is_term_page, $objs, $types, count($trail));
    $trail = array_merge($trail, $term_trail);
    // Optionally remove the current TERM from end of breadcrumb trail.
    if ((!variable_get('custom_breadcrumbs_taxonomy_show_current_term', TRUE) || ($is_term_page && !variable_get('custom_breadcrumbs_taxonomy_show_current_term_term', FALSE))) && (count($trail) > 1)) {
      array_pop($trail);
    }
  }
  return $trail;
}

/**
 * Translates configurable string when i18ntaxonomy module is installed.
 */
function _custom_breadcrumbs_taxonomy_tt($string_id, $default, $language = NULL) {
  return function_exists('tt') ? tt($string_id, $default, $language) : $default;
}

/**
 * Generates the home breadcrumb trail.
 *
 * @return
 *   The breadcrumb trail.
 */
function _custom_breadcrumbs_taxonomy_home_trail() {
  $trail = array();
  if (variable_get('custom_breadcrumbs_taxonomy_append_breadcrumb', FALSE)) {
    // Check to see if a breadcrumb has already been defined.
    $trail = drupal_get_breadcrumb();
  }
  else {
    $trail = custom_breadcrumbs_home_crumb();
  }
  return $trail;
}

/**
 * Generates the vocabulary trail.
 *
 * @param $vid
 *   A taxonomy vocabulary id.
 * @param $is_term_page
 *   TRUE if the breadcrumb is being prepared for the taxonomy term page, FALSE otherwise.
 * @param $objs
 *   An optional array of objects to be used to determine breadcrumb visibility and for token replacement.
 * @param $types
 *   An array of token types to be used in token replacement.
 * @param $part
 *   A postive integer indicating the breadcrumb segment (home crumb = 0).
 *
 * @return
 *   The breadcrumb trail.
 */
function _custom_breadcrumbs_taxonomy_vocabulary_trail($vid, $is_term_page = FALSE, $objs = array(), $types = array('global' => NULL), $part = 1) {
  // Generate the VOCABULARY breadcrumb.
  $trail = array();
  // Check to see if a vocabulary breadcrumb exists.
  $breadcrumbs = custom_breadcrumbs_load_breadcrumbs('custom_breadcrumbs_taxonomy', 'custom_breadcrumbs_taxonomy_vocabulary', array('vid' => $vid));
  $vocabulary_path = NULL;
  $title = NULL;
  $bid = NULL;
  if ($breadcrumb = custom_breadcrumbs_select_breadcrumb($breadcrumbs, $objs)) {
    $vocabulary_path = $breadcrumb->paths;
    $title = $breadcrumb->titles;
    $bid = $breadcrumb->bid;
    if (module_exists('token')) {
      $vocabulary_path = token_replace_multiple($vocabulary_path, $types);
      $title = token_replace_multiple($title, $types);
    }
  }
  if ($title == NULL) {
    $vocabulary = taxonomy_vocabulary_load($vid);
    $title = _custom_breadcrumbs_taxonomy_tt("taxonomy:vocabulary:$vid:name", $vocabulary->name);
  }
  if ($vocabulary_path != NULL) {
    $options = _custom_breadcrumbs_identifiers_option($part, $bid);
    $trail = array(l($title, $vocabulary_path, $options));
  }
  elseif (variable_get('custom_breadcrumbs_taxonomy_show_vocabulary', FALSE)) {
    $trail = array(check_plain($title));
  }
  return $trail;
}

/**
 * Generates the taxonomy term trail.
 *
 * @param $tid
 *   A taxonomy term id.
 * @param $is_term_page
 *   TRUE if the breadcrumb is being prepared for the taxonomy term page, FALSE otherwise.
 * @param $objs
 *   An optional array of objects to be used to determine breadcrumb visibility and for token replacement.
 * @param $types
 *   An array of token types to be used in token replacement.
 * @param $part
 *   A postive integer indicating the breadcrumb segment (home crumb = 0).
 *
 * @return
 *   The breadcrumb trail.
 */
function _custom_breadcrumbs_taxonomy_term_trail($tid, $is_term_page = FALSE, $objs = array(), $types = array('global' => NULL), $part = 1) {
  // Generate the TERM breadcrumb.
  $trail = array();
  $parent_terms = array_reverse(taxonomy_get_parents_all($tid));
  foreach ($parent_terms as $parent_term) {
    $breadcrumbs = custom_breadcrumbs_load_breadcrumbs('custom_breadcrumbs_taxonomy', 'custom_breadcrumbs_taxonomy_term', array('tid' => $parent_term->tid));
    $term_path = NULL;
    $title = NULL;
    $bid = NULL;
    if ($breadcrumb = custom_breadcrumbs_select_breadcrumb($breadcrumbs, $objs)) {
      $term_path = $breadcrumb->paths;
      $title = $breadcrumb->titles;
      $bid = $breadcrumb->bid;
      if (module_exists('token')) {
        $term_path = token_replace_multiple($term_path, $types);
        $title = token_replace_multiple($title, $types);
      }
    }
    if ($title == NULL) {
      $title = _custom_breadcrumbs_taxonomy_tt("taxonomy:term:$parent_term->tid:name", $parent_term->name);
    }
    if ($term_path == NULL) {
      $term_path = taxonomy_term_path(taxonomy_get_term($parent_term->tid));
    }

    // Do not create links to own self if we are on a taxonomy/term page.
    if ($is_term_page && $parent_term->tid == $tid) {
      $trail[] = check_plain($title);
    }
    else {
      $options = _custom_breadcrumbs_identifiers_option($part, $bid);
      $trail[] = l($title, $term_path, $options);
    }
    ++$part;
  }
  return $trail;
}
