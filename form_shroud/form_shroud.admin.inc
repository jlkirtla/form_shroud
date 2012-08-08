<?php

/**
 * @file
 * Provides forms for managing form shroud registry.
 */

/**
 * Form constructor for adding an entry to the form shroud registry.
 */
function form_shroud_settings_define(&$form) {
  if (!user_access('form_shroud bypass')) {
    drupal_access_denied();
  }
  $shroud_registry = db_select('form_shroud_registry', 'f')
    ->fields('f')
    ->execute()
    ->fetchAllAssoc('fsid');
  if (!user_access('form_shroud bypass')) {
    drupal_access_denied();
  }
  $registry_table = array(
    'header' => array(t('ID'), t('Title'), t('Path'), array('data' => t('Operations'), 'colspan' => 2)),
    'attributes' => array(),
    'caption' => '',
    'colgroups' => array(),
    'sticky' => '',
    'empty' => '',
    'rows' => array(),
  );
  foreach ($shroud_registry as $row) {
    $registry_table['rows'][$row->fsid] = array(
      $row->id,
      $row->title,
      l($row->path, $row->path),
      l(t('Edit'), 'admin/structure/formshroud/' . $row->fsid . '/edit'),
      l(t('Delete'), 'admin/structure/formshroud/' . $row->fsid . '/remove'),
    );
  }
  $form['form_shroud_registry_table'] = array(
    '#markup' => theme_table($registry_table),
  );
  return $form;
}

/**
 * Form constructor for the list of shroudable forms.
 */
function form_shroud_add(&$form, $form_state) {
  $form['form_shroud_add'] = array(
    '#type' => 'textfield',
    '#title' => t('Add Entity to Registry (by title)'),
    '#default_value' => '',
    '#size' => 60,
    '#maxlength' => 120,
    '#autocomplete_path' => 'formshroud/input',
  );
  $form['form_shroud_submit'] = array(
    '#type' => 'submit',
    '#value' => 'Add',
  );
  return $form;
}

/**
 * Form submission handler for form_shroud_add_entry().
 */
function form_shroud_add_submit(&$form, $form_state) {
  $data = explode(':', $form_state['values']['form_shroud_add']);
  switch ($data[0]) {
    case 'node':
      $node = db_select('node', 'n')
        ->fields('n')
        ->condition('n.nid', $data[1])
        ->execute()
        ->fetchAssoc();
      $path = db_select('url_alias', 'u')
        ->fields('u', array('alias'))
        ->condition('source', 'node/' . $node['nid'])
        ->execute()
        ->fetchAssoc();
      $fsid = db_insert('form_shroud_registry')
        ->fields(array(
          'id' => $node['nid'],
          'form_id' => 'node_form',
          'title' => $node['title'],
          'path' => (!empty($path['alias'])) ? $path['alias'] : 'node/' . $node->nid,
        ))
        ->execute();
        drupal_goto('admin/structure/formshroud/' . $fsid . '/edit');
      break;
    case 'menu_link':
      $menu_link = db_select('menu_links', 'm')
        ->fields('m')
        ->condition('m.mlid', $data[1])
        ->execute()
        ->fetchAssoc();
      $fsid = db_insert('form_shroud_registry')
        ->fields(array(
          'id' => $menu_link['mlid'],
          'form_id' => 'menu_overview_form',
          'title' => $menu_link['link_title'],
          'path' => $menu_link['link_path']
        ))
        ->execute();
        drupal_goto('admin/structure/formshroud/' . $fsid . '/edit');
      break;
    case 'menu':
      $fsid = db_insert('form_shroud_registry')
        ->fields(array(
          'form_id' => 'menu_edit_menu',
          'title' => $data[1],
        ))
        ->execute();
        drupal_goto('admin/structure/formshroud/' . $fsid . '/edit');
      break;
    default:
      drupal_set_message(t('Please use the "entity:id" format for the textfield.'));
  }
  
}

/**
 * Form constructor for the removal of an entry from the form shroud registry.
 *
 * @param int $fsid
 *   Unique form shroud ID to query the database.
 */
function form_shroud_settings_remove(&$form, $form_state, $fsid) {
  if (!user_access('form_shroud bypass')) {
    drupal_access_denied();
  }
  if (empty($fsid)) {
    drupal_set_message('Error in URL. Please go back to the previous page.');
    return;
  }
  $form['remove_check'] = array(
    '#markup' => t('Are you sure you want to delete this Form Shroud entry?'),
  );
  $form['remove_fsid'] = array(
    '#type' => 'hidden',
    '#value' => $fsid,
  );
  $form['remove_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
  );
  return $form;
}

/**
 * Form submission handler for form_shroud_settings_remove().
 */
function form_shroud_settings_remove_submit(&$form, $form_state) {
  if (db_delete('form_shroud_registry')
    ->condition('fsid', $form_state['values']['remove_fsid'])
    ->execute()) {
    drupal_set_message(t('Form Shroud entry deleted.'));
  } else {
    drupal_set_message(t('Error in entry deletion.'));
  }
  drupal_goto('admin/structure/formshroud');
}

/**
 * Form constructor for editing a node's shrouded form elements.
 *
 * @param int $fsid
 *   Uniuque ID to indicate which entity's form properties is being edited.
 */
function form_shroud_settings_edit(&$form, $form_state, $fsid) {
  if (!user_access('form_shroud bypass')) {
    drupal_access_denied();
  }
  if (empty($fsid)) {
    drupal_set_message(t('Error in URL. Please go back to the previous page.'));
    return;
  }
  $elements = module_invoke_all('shroud_define');
  $shroud_registry = db_select('form_shroud_registry', 'f')->fields('f')->condition('fsid', $fsid)->execute()->fetchAssoc();
  $opt = array_combine(array_keys($elements), array_keys($elements));
  $form['form_type'] = array(
    '#type' => 'select',
    '#title' => t('Form ID'),
    '#options' => $opt,
    '#default_value' => $shroud_registry['form_id'],
    '#description' => t('Set this to the form ID of the form you want shrouded.'),
  );
  foreach (array_keys($elements) as $edit_forms) {
    $form['field'][$edit_forms] = array(
      '#type' => 'fieldset',
      '#title' => t('Define (check) properties to be shrouded.'),
      '#states' => array(
        'visible' => array(
          ':input[name=form_type]' => array('value' => $edit_forms),
        ),
      ),
    );
    $reg = unserialize($shroud_registry['settings']);
    foreach ($elements[$edit_forms]['options'] as $source => $options) {
      foreach ($options as $machine_name => $title) {
        if (is_array($title)) {
          foreach ($title as $generic => $variable) {
            if (is_array($variable)) {
              foreach ($title as $organic => $var) {
                if (is_array($var)) {
                  foreach ($var as $index => $data) {
                    $form['field'][$edit_forms][$index] = array(
                      '#type' => 'checkbox',
                      '#title' => t($data),
                      '#default_value' => (isset($reg[$machine_name][$generic][$organic])) ? 1 : 0,
                      '#states' => array(
                        'visible' => array(
                          ':input[name=form_type]' => array('value' => $edit_forms),
                        ),
                      ),
                    );
                  }
                } else {
                  $form['field'][$edit_forms][$organic] = array(
                    '#type' => 'checkbox',
                    '#title' => t($var),
                    '#default_value' => (isset($reg[$machine_name][$generic][$organic])) ? 1 : 0,
                    '#states' => array(
                      'visible' => array(
                        ':input[name=form_type]' => array('value' => $edit_forms),
                      ),
                    ),
                  );
                }
              }
            }
            else {
              $form['field'][$edit_forms][$generic] = array(
                '#type' => 'checkbox',
                '#title' => t($variable),
                '#default_value' => (isset($reg[$machine_name][$generic])) ? 1 : 0,
                '#states' => array(
                  'visible' => array(
                    ':input[name=form_type]' => array('value' => $edit_forms),
                  ),
                ),
              );
            }
          }
        }
        else {
          $form['field'][$edit_forms][$machine_name] = array(
            '#type' => 'checkbox',
            '#title' => t($title),
            '#default_value' => (isset($reg[$machine_name])) ? 1 : 0,
            '#states' => array(
              'visible' => array(
                ':input[name=form_type]' => array('value' => $edit_forms),
              ),
            ),
          );
        }
      }
    }
  }
  $form['fsid'] = array(
    '#type' => 'hidden',
    '#value' => $fsid,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );
  return $form;
}

/**
 * Form submission handler for form_shroud_settings_edit().
 */
function form_shroud_settings_edit_submit($form, $form_state) {
  $shroud_registry = db_select('form_shroud_registry', 'f')->fields('f')->condition('fsid', $form_state['values']['fsid'])->execute()->fetchAssoc();
  $elements = module_invoke_all('shroud_define');

  foreach ($elements[$shroud_registry['form_id']]['options'] as $key => $form_element) {
    foreach ($form_element as $machine_name => $property) {
      if (is_array($property)) {
        foreach ($property as $gen => $var) {
          if (is_array($var)) {
            foreach ($var as $key_value => $stored_value) {
              if (is_array($stored_value)) {
                foreach ($stored_value as $another_key => $more_values) {
                  if ($form_state['values'][$another_key]) {
                    $insert[$machine_name][$gen][$key_value][$another_key] = 1;
                  }
                }
              }
              else {
                if ($form_state['values'][$key_value]) {
                  $insert[$machine_name][$gen][$key_value] = 1;
                }
              }
            }
          }
          else {
            if ($form_state['values'][$gen]) {
              $insert[$machine_name][$gen] = 1;
            }
          }
        }
      }
      else {
        if ($form_state['values'][$machine_name]) {
          $insert[$machine_name] = 1;
        }
      }
    }
  }
  $settings = (!empty($insert)) ? serialize($insert) : '';
  drupal_set_message(t('The form elements have been shrouded.'));
  db_update('form_shroud_registry')->fields(array('settings' => $settings, 'form_id' => $form_state['values']['form_type']))->condition('fsid', $form_state['values']['fsid'])->execute();
  drupal_goto('admin/structure/formshroud');
}

/**
 * Autocomplete menu callback function.
 */
function form_shroud_json_callback($string) {
  $matches = array();
  
  $list['node'] = db_select('node', 'n')
    ->fields('n')
    ->condition('n.title', '%' . db_like($string) . '%', 'LIKE')
    ->range(0, 5)
    ->execute()
    ->fetchAllAssoc('nid');
  $list['menu_link'] = db_select('menu_links', 'm')
    ->fields('m')
    ->condition('m.link_title', '%' . db_like($string) . '%', 'LIKE')
    ->range(0, 5)
    ->execute()
    ->fetchAllAssoc('mlid');
  $list['menu'] = db_select('menu_custom', 'm')
    ->fields('m')
    ->condition(db_or()
      ->condition('m.title', '%' . db_like($string) . '%', 'LIKE')
      ->condition('m.menu_name', '%' . db_like($string) . '%', 'LIKE')
    )
    ->range(0, 5)
    ->execute()
    ->fetchAllAssoc('menu_name');
  foreach ($list as $type => $queries) {
    foreach ($queries as $row) {
      switch ($type) {
        case 'node':
          $matches['node:' . $row->nid] = 'Node: ' . $row->title;
          break;
        case 'menu_link':
          $matches['menu_link:' . $row->mlid] = 'Menu Link: ' . $row->menu_name . ':' . $row->link_title;
          break;
        case 'menu':
          $matches['menu:' . $row->title] = 'Menu: ' . $row->title;
          break; 
      }
    }
  }
  drupal_json_output($matches);
}
