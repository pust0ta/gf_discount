<?php

/**
 * @file
 * Функции, вызываемые на страницах административного управления модулем.
 *
 * @see gf_discount_menu()
 */

function gf_discount_settings_form(){
  $form = [];
  $defaults = [ // сумма в тыс. руб. => скидка(в процентах)
    50 => 5, 
    100 => 10
  ];
  $settings = variable_get('gf_discount_newbie_discounts', $defaults);
  $levels = range(1, count($settings));

  $form['newbie_discount'] = [
    '#type' => 'fieldset',
    '#title' => t('Newbie discounts'),
    '#description' => t('Setting discounts for a newbie.'),
  ];

  $i = 0;
  foreach($levels as $level) {
    $form['newbie_discount']['level_'. $level] = [
      '#type' => 'fieldset',
      '#title' => t('Level %level Discount', ['%level' => $level]),
      '#description' => t('Discount for the level %level', ['%level' => $level]),
    ];
    // Настройка суммы для уровня.
    $form['newbie_discount']['level_' . $level]['newbie_level_' . $level . '_sum'] = [
      '#type' => 'textfield',
      '#title' => t('Required amount (in thousands of rubles)'),
      '#description' => t('The minimum order amount required to receive a discount.'),
      '#default_value' => array_keys($settings)[$i],
      '#required' => true,
      '#element_validate' => array('element_validate_integer_positive'), 
    ];
    // Настройка скидки для указанной выше суммы.
    $form['newbie_discount']['level_' . $level]['newbie_level_' . $level . '_discount'] = [
      '#type' => 'textfield',
      '#title' => t('Discount (in percent)'),
      '#description' => t(''),
      '#default_value' => array_values($settings)[$i],
      '#required' => true,
      '#element_validate' => array(
        'element_validate_integer_positive', 
        'gf_discount_percent_validate'
      ), 
      '#maxlength' => 3,
    ];

    $i++;
  }
  $form['#newbie_discount_levels'] = $levels;

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
  ];

  return $form;
}

/**
 * Implements hook_form_submit()
 *
 * Собираем настройки по дисконтам для новичков в один массив.
 */
function gf_discount_settings_form_submit($form, &$form_state){
  $levels = $form['#newbie_discount_levels'];
  $values = $form_state['values'];
  $settings = [];
  foreach ($levels as $level) {
    $settings[$values['newbie_level_'. $level . '_sum']] = $values['newbie_level_' . $level . '_discount'];
  }
  variable_set('gf_discount_newbie_discounts', $settings);
}

/**
 * Проверка на правильный ввод процентов.
 *
 */
function gf_discount_percent_validate($element, $form_state) {
  if ($element['#value'] > 100) {
    form_error($element, t('The value must be less than or equal to 100.'));
  }
}


/**
 * Форма для настройки скидок для нужных ролей.
 *
 */
function gf_discount_roles_settings_form() {
  $form = [];
  $settings = variable_get('gf_discount_role_discounts', []);

  $form['summarize_discounts'] = [
    '#type' => 'checkbox',
    '#title' => t('Summarize discounts'),
    '#description' => t('Summarize discounts if the user has several roles to which different discounts apply. If the check box is not checked, the maximum discount will be selected.'),
    '#default_value' => variable_get('gf_discount_summarize_discounts', FALSE),
  ];

  $form['discount_by_role'] = [
    '#type' => 'fieldset',
    '#title' => t('Discount by role'),
    '#description' => t('Setting up discounts for custom roles')
  ];

  $roles = user_roles(TRUE);
  ksort($roles);
  foreach($roles as $rid => $role) {
    $form['discount_by_role']['role_' . $rid] = [
        '#type' => 'textfield',
        '#title' => $role . ' (' . $rid . ')',
        '#description' => t('Set the discount in percent.'),
        '#default_value' => isset($settings[$rid]) ? $settings[$rid] : 0,
        '#element_validate' => array(
          'element_validate_integer', 
          'gf_discount_percent_validate'
        ), 
        '#maxlength' => 3,
        '#width' => 4,
    ];
  }

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
  ];

  return $form;
}


/**
 * Сохранение настроек по скидкам для ролей.
 *
 */
function gf_discount_roles_settings_form_submit($form, &$form_state) {
  $discounts = [];
  foreach($form_state['values'] as $key => $val) {
    if ($key == 'summarize_discounts') {
      variable_set('gf_discount_summarize_discounts', $val);
    }
    elseif (substr($key, 0, 5) === 'role_' && $val > 0) {
      $role_id = (int) str_replace('role_', '', $key);
      $discounts[$role_id] = $val;
    }
  }

  variable_set('gf_discount_role_discounts', $discounts);
}


/**
 * Настройка скидок на основе суммы по заказам за прошлый месяц.
 *
 */
function gf_discount_previous_month_based_settings_form($form, &$form_state){
  $settings = variable_get('gf_discount_prev_month_based_discounts', '');
  if (is_array($settings)) {
    $settings = implode("\n", array_map(
      function ($v, $k) {
        return sprintf("%s|%s", $k, $v);
      }, $settings, array_keys($settings)
    ));
  }
  $form['min_max_proc'] = [
    '#type' => 'textarea',
    '#title' => t('Required amount (in thousands of rubles)'),
    '#description' => t('The minimum order amount required to receive a discount.'),
    '#default_value' => $settings,
    '#required' => true,
    // '#element_validate' => array('element_validate_integer_positive'), 
  ];
  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
  ];
  return $form;
}

function gf_discount_previous_month_based_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $levels = explode("\n", $values['min_max_proc']);
  $settings = [];

  foreach($levels as $level) {
    list($sum, $percent) = explode('|', $level);
    $settings[$sum] = $percent;
  }

  variable_set('gf_discount_prev_month_based_discounts', $settings);
}
