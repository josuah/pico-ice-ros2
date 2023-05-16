from setuptools import setup

package_name = 'pico_ice'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='build',
    maintainer_email='build@todo.todo',
    description='TODO: Package description',
    license='MIT',
    tests_require=['pytest'],
	entry_points={
        'console_scripts': [
            'talker = pico_ice.publisher_member_function:main',
            'listener = pico_ice.subscriber_member_function:main',
        ],
	},
)
