<?php

function beautytips_textinput_admin_info() {
  $form['beautytips_text'] = array(
    '#type' => 'fieldset',
    '#title' => t('Text Input Tooltips'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['beautytips_text']['beautytips_text_input'] = array(
    '#type' => 'checkbox',
    '#title' => 'Display text input popups',
    '#default_value' => variable_get('beautytips_text_input', FALSE),
  );
  $form['beautytips_text']['beautytips_select_input'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display select box input popups'),
    '#default_value' => variable_get('beautytips_select_input', FALSE),
  );
  $form['beautytips_text']['beautytips_position'] = array(
    '#type' => 'radios',
    '#title' => t('Which side should the text popups appear on?'),
    '#options' => array('bottom' => t('bottom'), 'top' => t('top'), 'left' => t('left'), 'right' => t('right')),
    '#default_value' => variable_get('beautytips_position', 'bottom'),
  );
  $form['beautytips_text']['beautytips_text_trigger'] = array(
    '#type' => 'radios', 
    '#title' => t('What should trigger the popup?'),
    '#options' => array('hover' => t('hover'), 'focus' => t('focus')),
    '#default_value' => variable_get('beautytips_text_trigger', 'focus'),
  );      
  $form['beautytips_text']['form_id'] = array(
    '#type' => 'fieldset',
    '#title' => t('Restrict text popups to specific forms. (OPTIONAL)'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,    
  );
  $form['beautytips_text']['form_id']['beautytips_form_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter the form id(s) to use for text popup beautytips'),
    '#description' => t('You need to use Drupal php syntax like page_node_form'),
    '#default_value' => variable_get('beautytips_form_id', ''),
  );
  $form['beautytips_text']['form_id']['beautytips_show_form'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display form_ids'),
    '#description' => t("Turn this on if you want to see the names of the form ids to enter above.  The names will appear on the pages where the forms are displayed.") . '<div>' . t("Make sure that you turn it off when you're done retrieving the form_ids.") . '</div>',
    '#default_value' => variable_get('beautytips_show_form', ''),
  );
  
  return $form;
}

function beautytips_textinput_menu_items() {
  $items['beautytips/settings/form_ids'] = array(
    'page callback' => 'beautytips_textinput_form_id_off',
    'access arguments' => array('administer site configuration'),
    'file' => 'textinput.inc',
    'file path' => drupal_get_path('module', 'beautytips_ui') . '/includes',
    'type' => MENU_CALLBACK,
  );
  return $items;
}

function beautytips_textinput_form_change(&$form, $form_state, $form_id) {
  if (variable_get('beautytips_text_input', FALSE) || variable_get('beautytips_select_input', FALSE)) {
    if (variable_get('beautytips_show_form', FALSE)) {
      drupal_set_message(t('The form_id is %form_id.  This message should be ' . l(t('turned off'), 'beautytips/settings/form_ids', array('query' => drupal_get_destination())) . " when finished you're finished checking form_ids
.", array('%form_id' => $form_id)));
    }
     
    $add_bt = TRUE;
    if (strlen(variable_get('beautytips_form_id', ''))) {
      if (strpos(variable_get('beautytips_form_id', ''), $form_id) === FALSE) {
        $add_bt = FALSE;
      }
    } 
    if ($add_bt) {
      $options = array();
      $trigger = variable_get('beautytips_text_trigger', 'focus') == 'hover' ? 'hover' : array('focus', 'dblclick');
      if (variable_get('beautytips_text_input', FALSE)) {
        $options['bt_text_field'] = array(
          'cssSelect' => 'input.form-text',
          'trigger' => $trigger,
          'contentSelector' => "$(this).nextAll('.description:eq(0)').hide().html()",
          'shrinkToFit' => TRUE,
          'positions' => array(0 => variable_get('beautytips_position', 'bottom')),
          'preEval' => TRUE,
        );
        $options['bt_text_area'] = array(
          'cssSelect' => 'textarea.form-textarea',
          'trigger' => $trigger,
          'contentSelector' => "$(this).parents('.form-item').find('.description').hide().html()",
          'shrinkToFit' => TRUE,
          'positions' => array(0 => variable_get('beautytips_position', 'bottom')),
          'preEval' => TRUE,
        );
      }
      if (variable_get('beautytips_select_input', FALSE)) {
        $options['bt_select_element'] = array(
          'cssSelect' => '.form-select',
          'contentSelector' => "$(this).parents('.form-item').find('.description').hide().html()",
          'trigger' => $trigger,
          'shrinkToFit' => TRUE,
          'preEval' => TRUE,
        );
        $options['bt_checkbox_element'] = array(
          'cssSelect' => 'label.option',
          'contentSelector' => "$(this).parents('.form-item').find('.description').hide().html()",
          'trigger' => $trigger,
          'shrinkToFit' => TRUE,
          'preEval' => TRUE,
        );
      }
      beautytips_add_beautytips($options);
    }
  }  
}


/**
 * Menu callback.  Turns off the display of form-ids.
 */
function beautytips_textinput_form_id_off() {
  variable_set('beautytips_show_form', FALSE);
  $destination = drupal_get_destination() ? drupal_get_destination() : 'admin/settings/beautytips';
  drupal_goto($destination);
}

