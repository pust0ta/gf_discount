<?php

/**
 * @file
 * Функции, вызываемые на страницах административного управления модулем.
 *
 * @see gf_discount_menu()
 */

function gf_discount_settings_form($form, &$form_state){
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