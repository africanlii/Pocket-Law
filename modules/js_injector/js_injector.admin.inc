<?php

/**
 * @file
 * Administration pages for js_injector.
 */


/**
 * Page callback for js Injector's main admin page.
 */
function js_injector_admin_form() {
  $rules = _js_injector_load_rule(NULL, TRUE);
  $path = drupal_get_path('module', 'js_injector') .'/';
  $form = array();
  $form['#tree'] = TRUE;
  foreach ($rules as $rule) {
    $form['rules'][$rule['crid']]['#rule'] = $rule;
    $form['rules'][$rule['crid']]['edit'] = array(
      '#type' => 'image_button',
      '#title' => t('Edit rule'),
      '#src' => $path .'text-editor.png',
      '#submit' => array('js_injector_admin_edit_button'),
      '#crid' => $rule['crid'],
    );
    $form['rules'][$rule['crid']]['delete'] = array(
      '#type' => 'image_button',
      '#title' => t('Delete rule'),
      '#src' => $path .'edit-delete.png',
      '#submit' => array('js_injector_admin_delete_button'),
      '#crid' => $rule['crid'],
    );
  }
  return $form;
}

/**
 * Edit button callback for the js rule listing form.
 */
function js_injector_admin_edit_button($form, &$form_state) {
  $button = $form_state['clicked_button'];
  $crid = $button['#crid'];
  $form_state['redirect'] = 'admin/settings/js_injector/edit/'. $crid;
}

/**
 * Delete button callback for the js rule listing form.
 * Redirects the user to the confirmation form.
 */
function js_injector_admin_delete_button($form, &$form_state) {
  $button = $form_state['clicked_button'];
  $crid = $button['#crid'];
  $form_state['redirect'] = 'admin/settings/js_injector/delete/'. $crid;
}

/**
 * Theme function for the js Injector admin overview form.
 */
function theme_js_injector_admin_form($form) {
  $headers = array(t('Title'), t('File path'), t('Actions'));
  $rows = array();
  if (!empty($form['rules'])) {
    foreach (element_children($form['rules']) as $crid) {
      $row = array();
      $rule = $form['rules'][$crid]['#rule'];
      $row[] = check_plain($rule['title']);
      $row[] = check_plain(_js_injector_rule_path($rule['crid']));
      $row[] = drupal_render($form['rules'][$crid]);
      $rows[] = $row;
    }
  }

  $link = l(t('Create a new rule'), 'admin/settings/js_injector/add');
  $row = array();
  if (empty($rows)) {
    $row[] = array(
      'data' => t('No js injection rules have been set up yet. !url.', array('!url' => $link)),
      'colspan' => 3,
    );
  }
  else {
    $row[] = array(
      'data' => t('!url.', array('!url' => $link)),
      'colspan' => 3,
    );
  }
  $rows[] = $row;

  $output .= theme('table', $headers, $rows);
  $output .= drupal_render($form);
  return $output;
}

/**
 * Constructor for the js rule edit form.
 */
function js_injector_edit($form_state, $crid = NULL) {
  if (isset($crid)) {
    $rule = _js_injector_load_rule($crid, TRUE);
    $form['crid'] = array(
      '#type' => 'value',
      '#value' => $crid,
    );

    $path = file_create_path(_js_injector_rule_path($rule['crid']));
    if (file_exists($path)) {
      $rule['js_text'] = file_get_contents($path);
    }
    else {
      $rule['js_text'] = '';
    }
  }
  else {
    $rule = array(
      'title' => '',
      'rule_type' => 0,
      'rule_conditions' => '',
      'scope' => 'header',
      'preprocess' => 1,
      'cache' => 1,
      'js_text' => '',
    );
  }

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $rule['title'],
    '#required' => TRUE,
  );

  $form['js_text'] = array(
    '#type' => 'textarea',
    '#title' => t('js code'),
    '#rows' => 10,
    '#default_value' => $rule['js_text'],
    '#required' => TRUE,
  );

  // Shamelessly ripped from block.module. Who doesn't use this snippet
  // of code, really?
  $access = user_access('use PHP for block visibility');
  if ($rule['rule_type'] == 2 && !$access) {
    $form['conditional'] = array();
    $form['conditional']['rule_type'] = array('#type' => 'value', '#value' => 2);
    $form['conditional']['rule_conditions'] = array('#type' => 'value', '#value' => $rule['rule_conitions']);
  }
  else {
    $options = array(t('Add on every page except the listed pages.'), t('Add on only the listed pages.'));
    $description = t("Enter one page per line as Drupal paths. The '*' character is a wildcard. Example paths are %blog for the blog page and %blog-wildcard for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wildcard' => 'blog/*', '%front' => '<front>'));

    if ($access) {
      $options[] = t('Add if the following PHP code returns <code>TRUE</code> (PHP-mode, experts only).');
      $description .= ' '. t('If the PHP-mode is chosen, enter PHP code between %php. Note that executing incorrect PHP-code can break your Drupal site.', array('%php' => '<?php ?>'));
    }
    $form['conditional']['rule_type'] = array(
      '#type' => 'radios',
      '#title' => t('Add the js on specific pages'),
      '#options' => $options,
      '#default_value' => $rule['rule_type'],
    );
    $form['conditional']['rule_conditions'] = array(
      '#type' => 'textarea',
      '#title' => t('Pages'),
      '#default_value' => $rule['rule_conditions'],
      '#description' => $description,
    );
  }

  $form['scope'] = array(
    '#type' => 'select',
    '#title' => t('Scope'),
    '#options' => array('header' => t('Header'), 'footer' => t('Footer')),
    '#default_value' => $rule['scope'],
  );

  $form['preprocess'] = array(
    '#type' => 'checkbox',
    '#title' => t('Preprocess js'),
    '#default_value' => $rule['preprocess'],
  );
  
  $form['cache'] = array(
    '#type' => 'checkbox',
    '#title' => t('Cache js'),
    '#default_value' => $rule['cache'],
  );

  $form['buttons']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('js_injector_edit_save'),
  );

  $form['buttons']['save_and_continue'] = array(
    '#type' => 'submit',
    '#value' => t('Save and Continue'),
    '#submit' => array('js_injector_edit_save_and_continue'),
  );

  if (!empty($rule['crid'])) {
    $form['buttons']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('js_injector_admin_delete_button'),
      '#crid' => $rule['crid'],
    );
  }

  return $form;
}

/**
 * Validation callback for the js rule edit form.
 */
function js_injector_edit_validate($form, &$form_state) {
  $rule = $form_state['values'];
  $directory = file_directory_path();
  file_check_directory($directory, 1);
  $form_state['rule'] = $rule;
}

/**
 * Save and continue callback for the CSS rule edit form.
 */
function js_injector_edit_save_and_continue($form, &$form_state) {
  js_injector_edit_save($form, $form_state);
  $form_state['redirect'] = 'admin/settings/js_injector/edit/' . $form_state['rule']['crid'];
}

/**
 * Submit button callback for the js rule edit form.
 */
function js_injector_edit_save($form, &$form_state) {
  $rule = $form_state['rule'];

  drupal_write_record('js_injector_rule', $rule, empty($rule['crid']) ? NULL : 'crid');

  file_save_data($rule['js_text'], file_create_path(_js_injector_rule_path($rule['crid'])), FILE_EXISTS_REPLACE);
  _js_injector_load_rule(NULL, TRUE);

  drupal_set_message('Your js injection rule was saved.');
  $form_state['redirect'] = 'admin/settings/js_injector';
}


/**
 * Menu callback -- ask for confirmation of rule deletion.
 */
function js_injector_delete_confirm(&$form_state, $crid) {
  $form['crid'] = array(
    '#type' => 'value',
    '#value' => $crid,
  );

  $rule = _js_injector_load_rule($crid);
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $rule['title'])),
    isset($_GET['destination']) ? $_GET['destination'] : 'admin/settings/js_injector',
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Execute node deletion.
 */
function js_injector_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    _js_injector_delete_rule($form_state['values']['crid']);
  }

  $form_state['redirect'] = 'admin/settings/js_injector';
  return;
}
